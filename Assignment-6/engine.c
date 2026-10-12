#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

static int count_matches(const char *line, const char *target) {
	size_t target_length = strlen(target);
	int count = 0;

	if (target_length == 0) {
		return 0;
	}

	for (const char *position = line; *position != '\0'; position++) {
		if (strncmp(position, target, target_length) != 0) {
			continue;
		}

		count++;
		position += target_length - 1;
	}

	return count;
}

int search_count(char *filename, char *target) {
	FILE *file = fopen(filename, "r");
	if (file == NULL || target == NULL) {
		if (file != NULL) {
			fclose(file);
		}
		return 0;
	}

	char *line = NULL;
	size_t capacity = 0;
	ssize_t length;
	int count = 0;

	while ((length = getline(&line, &capacity, file)) != -1) {
		(void)length;
		count += count_matches(line, target);
	}

	free(line);
	fclose(file);
	return count;
}

struct count_result search_instance(char *filename,char *target){
	struct count_result result = {0, NULL};
	FILE *file = fopen(filename, "r");
	if (file == NULL || target == NULL) {
		if (file != NULL) {
			fclose(file);
		}
		return result;
	}

	char *line = NULL;
	size_t capacity = 0;
	ssize_t length;

	while ((length = getline(&line, &capacity, file)) != -1) {
		int matches = count_matches(line, target);
		if (matches == 0) {
			continue;
		}

		size_t start = 0;
		size_t end = (size_t)length;
		while (start < end && isspace((unsigned char)line[start])) {
			start++;
		}
		while (end > start && isspace((unsigned char)line[end - 1])) {
			end--;
		}

		for (int match = 0; match < matches; match++) {
			char **instances = realloc(result.instances,
				(size_t)(result.count + 1) * sizeof(*result.instances));
			if (instances == NULL) {
				goto finished;
			}
			result.instances = instances;

			size_t line_length = end - start;
			result.instances[result.count] = malloc(line_length + 1);
			if (result.instances[result.count] == NULL) {
				goto finished;
			}
			memcpy(result.instances[result.count], line + start, line_length);
			result.instances[result.count][line_length] = '\0';
			result.count++;
		}
	}

finished:
	free(line);
	fclose(file);
	return result;
}