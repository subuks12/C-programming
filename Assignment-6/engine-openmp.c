#include "engine.h"
#include <ctype.h>
#include <fcntl.h>
#include <omp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_WORKERS 8

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

static struct count_result scan_chunk(const char *data, size_t file_size,
									  size_t start, size_t end, const char *target,
									  size_t target_length,
									  bool collect_instances) {
	struct count_result result = {0, NULL};
	size_t position = start;

	while (position < end) {
		const char *candidate = memchr(data + position,
									   (unsigned char)target[0], end - position);
		if (candidate == NULL) {
			break;
		}

		size_t candidate_position = (size_t)(candidate - data);
		if (target_length > file_size - candidate_position ||
			memchr(candidate, '\n', target_length) != NULL ||
			memcmp(candidate, target, target_length) != 0) {
			position = candidate_position + 1;
			continue;
		}

		if (collect_instances) {
			size_t line_start = candidate_position;
			while (line_start > 0 && data[line_start - 1] != '\n') {
				line_start--;
			}
			size_t line_end = candidate_position;
			while (line_end < file_size && data[line_end] != '\n') {
				line_end++;
			}

			size_t trim_start = line_start;
			size_t trim_end = line_end;
			while (trim_start < trim_end &&
				   isspace((unsigned char)data[trim_start])) {
				trim_start++;
			}
			while (trim_end > trim_start &&
				   isspace((unsigned char)data[trim_end - 1])) {
				trim_end--;
			}
			if (!append_instance(&result, data + trim_start,
								 trim_end - trim_start)) {
				return result;
			}
		} else {
			result.count++;
		}
		position = candidate_position + target_length;
	}

	return result;
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
	int thread_count = (int)available_workers;
	size_t target_length = strlen(target);
	size_t starts[MAX_WORKERS + 1] = {0};
	struct count_result partial[MAX_WORKERS] = {0};

	for (int index = 1; index < thread_count; index++) {
		size_t boundary = (file_size / (size_t)thread_count) * (size_t)index +
			((file_size % (size_t)thread_count) * (size_t)index) /
				(size_t)thread_count;
		while (boundary < file_size && boundary > 0 &&
			   data[boundary - 1] != '\n') {
			boundary++;
		}
		if (boundary < starts[index - 1]) {
			boundary = starts[index - 1];
		}
		starts[index] = boundary;
	}
	starts[thread_count] = file_size;

	#pragma omp parallel for num_threads(thread_count) schedule(static)
	for (int index = 0; index < thread_count; index++) {
		partial[index] = scan_chunk(data, file_size, starts[index],
					    starts[index + 1], target, target_length,
					    collect_instances);
	}

	if (!collect_instances) {
		for (int index = 0; index < thread_count; index++) {
			result.count += partial[index].count;
		}
	} else {
		for (int index = 0; index < thread_count; index++) {
			if (partial[index].count == 0) {
				continue;
			}

			size_t combined_count = (size_t)result.count +
				(size_t)partial[index].count;
			char **instances = realloc(result.instances,
				combined_count * sizeof(*result.instances));
			if (instances == NULL) {
				free_instances(&partial[index]);
				continue;
			}
			result.instances = instances;
			memcpy(result.instances + result.count, partial[index].instances,
				   (size_t)partial[index].count * sizeof(*partial[index].instances));
			result.count += partial[index].count;
			free(partial[index].instances);
			partial[index].instances = NULL;
			partial[index].count = 0;
		}

		for (int index = 0; index < thread_count; index++) {
			free_instances(&partial[index]);
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