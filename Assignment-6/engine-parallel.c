#include "engine.h"
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_WORKERS 8

struct worker_args {
	const char *data;
	const char *target;
	size_t target_length;
	size_t start;
	size_t end;
	bool collect_instances;
	struct count_result result;
};

static bool append_instance(struct count_result *result, const char *line,
							size_t length) {
	char **instances = realloc(result->instances,
		(size_t)(result->count + 1) * sizeof(*result->instances));
	if (instances == NULL) {
		return false;
	}
	result->instances = instances;

	char *instance = malloc(length + 1);
	if (instance == NULL) {
		return false;
	}
	memcpy(instance, line, length);
	instance[length] = '\0';
	result->instances[result->count++] = instance;
	return true;
}

static void *search_worker(void *argument) {
	struct worker_args *args = argument;
	size_t line_start = args->start;

	while (line_start < args->end) {
		size_t line_end = line_start;
		while (line_end < args->end && args->data[line_end] != '\n') {
			line_end++;
		}

		size_t line_length = line_end - line_start;
		size_t position = 0;
		while (position + args->target_length <= line_length) {
			if (memcmp(args->data + line_start + position, args->target,
					   args->target_length) != 0) {
				position++;
				continue;
			}

			if (args->collect_instances) {
				size_t trim_start = 0;
				size_t trim_end = line_length;
				while (trim_start < trim_end &&
					   isspace((unsigned char)args->data[line_start + trim_start])) {
					trim_start++;
				}
				while (trim_end > trim_start &&
					   isspace((unsigned char)args->data[line_start + trim_end - 1])) {
					trim_end--;
				}
				if (!append_instance(&args->result,
									 args->data + line_start + trim_start,
									 trim_end - trim_start)) {
					return NULL;
				}
			} else {
				args->result.count++;
			}
			position += args->target_length;
		}

		line_start = line_end + 1;
	}

	return NULL;
}

static void free_instances(struct count_result *result) {
	for (int index = 0; index < result->count; index++) {
		free(result->instances[index]);
	}
	free(result->instances);
	result->instances = NULL;
	result->count = 0;
}

static struct count_result parallel_search(char *filename, char *target,
										   bool collect_instances) {
	struct count_result result = {0, NULL};
	if (filename == NULL || target == NULL || target[0] == '\0') {
		return result;
	}

	int file_descriptor = open(filename, O_RDONLY);
	if (file_descriptor == -1) {
		return result;
	}

	struct stat file_status;
	if (fstat(file_descriptor, &file_status) != 0 || file_status.st_size == 0) {
		close(file_descriptor);
		return result;
	}

	size_t file_size = (size_t)file_status.st_size;
	const char *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE,
							file_descriptor, 0);
	close(file_descriptor);
	if (data == MAP_FAILED) {
		return result;
	}

	long available_workers = sysconf(_SC_NPROCESSORS_ONLN);
	if (available_workers < 1) {
		available_workers = 1;
	}
	if (available_workers > MAX_WORKERS) {
		available_workers = MAX_WORKERS;
	}
	size_t worker_count = (size_t)available_workers;

	struct worker_args args[MAX_WORKERS] = {0};
	pthread_t threads[MAX_WORKERS];
	bool started[MAX_WORKERS] = {false};
	size_t target_length = strlen(target);

	args[0].start = 0;
	for (size_t index = 1; index < worker_count; index++) {
		size_t boundary = (file_size / worker_count) * index +
			((file_size % worker_count) * index) / worker_count;
		while (boundary < file_size && boundary > 0 &&
			   data[boundary - 1] != '\n') {
			boundary++;
		}
		if (boundary < args[index - 1].start) {
			boundary = args[index - 1].start;
		}
		args[index].start = boundary;
	}

	for (size_t index = 0; index < worker_count; index++) {
		args[index].data = data;
		args[index].target = target;
		args[index].target_length = target_length;
		args[index].end = index + 1 < worker_count
			? args[index + 1].start : file_size;
		args[index].collect_instances = collect_instances;

		if (pthread_create(&threads[index], NULL, search_worker,
						   &args[index]) == 0) {
			started[index] = true;
		} else {
			search_worker(&args[index]);
		}
	}

	for (size_t index = 0; index < worker_count; index++) {
		if (started[index]) {
			pthread_join(threads[index], NULL);
		}
	}

	if (!collect_instances) {
		for (size_t index = 0; index < worker_count; index++) {
			result.count += args[index].result.count;
		}
	} else {
		for (size_t index = 0; index < worker_count; index++) {
			struct count_result *partial = &args[index].result;
			if (partial->count == 0) {
				continue;
			}

			size_t combined_count = (size_t)result.count + (size_t)partial->count;
			char **instances = realloc(result.instances,
				combined_count * sizeof(*result.instances));
			if (instances == NULL) {
				free_instances(partial);
				continue;
			}
			result.instances = instances;
			memcpy(result.instances + result.count, partial->instances,
				   (size_t)partial->count * sizeof(*partial->instances));
			result.count += partial->count;
			free(partial->instances);
			partial->instances = NULL;
			partial->count = 0;
		}

		for (size_t index = 0; index < worker_count; index++) {
			free_instances(&args[index].result);
		}
	}

	munmap((void *)data, file_size);
	return result;
}

int search_count(char *filename, char *target) {
	return parallel_search(filename, target, false).count;
}

struct count_result search_instance(char *filename, char *target) {
	return parallel_search(filename, target, true);
}