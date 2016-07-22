#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>

enum json_error {
	JSON_ERROR_OK = 0,
	JSON_ERROR_UNEXPECTED_END,
	JSON_ERROR_UNEXPECTED_TOKEN
};

enum json_type {
	JSON_TYPE_UNDEFINED, // pseudotype
	JSON_TYPE_STRING,
	JSON_TYPE_NUMBER,
	JSON_TYPE_OBJECT,
	JSON_TYPE_ARRAY,
	JSON_TYPE_BOOLEAN,
	JSON_TYPE_NULL,
};

struct whitespace {
	char* content;
	size_t length;
};

struct json_string {
	char* content;
	size_t length;
};

struct json_number {
	char* content;
	size_t length;
};

struct json_object {
	struct json_string* keys;
	struct json_value* values;
	size_t length;
//	struct whitespace* whitespaces;
//	unsigned int whitespaces_length;
};

struct json_array {
	struct json_value* values;
	size_t length;
//	struct whitespace* whitespaces;
//	unsigned int whitespaces_length;
};

struct json_boolean {
	int value;
};

struct json_value {
	enum json_type type;
	union {
		struct json_string string;
		struct json_number number;
		struct json_boolean boolean;
		struct json_array array;
		struct json_object object;
	} value;
};


/***************************/
/** Buffer implementation **/
/***************************/


struct buffer {
	char* content;
	unsigned int length, size;
};


void buffer_append(struct buffer* buffer, const char* content, unsigned int length) {
	if(buffer->size < length + buffer->length) {
		if(buffer->size == 0)
			buffer->content = malloc(buffer->size = 4);
		else
			buffer->content = realloc(buffer->content, buffer->size *= 2);
	}

	memcpy(buffer->content + buffer->length, content, length);

	buffer->length += length;
}


void buffer_append_char(struct buffer* buffer, char content) {
	buffer_append(buffer, &content, 1);
}


/*****************/
/** JSON parser **/
/*****************/


enum json_error json_parser_scan_whitespace(const char** in, const char* end, struct whitespace* out);
enum json_error json_parser_scan_value(const char** in, const char* end, struct json_value* out);
enum json_error json_parser_scan_string(const char** in, const char* end, struct json_string* out);
enum json_error json_parser_scan_number(const char** in, const char* end, struct json_number* out);
enum json_error json_parser_scan_object(const char** in, const char* end, struct json_object* out);
enum json_error json_parser_scan_array(const char** in, const char* end, struct json_array* out);
enum json_error json_parser_scan_boolean(const char** in, const char* end, struct json_boolean* out);
enum json_error json_parser_scan_null(const char** in, const char* end);


enum json_error json_parser_scan_whitespace(const char** in, const char* end, struct whitespace* out) {

	assert(*in < end);

	struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };

	while(*in < end) {
		if(**in != 0x20 && **in != 0x09 && **in != 0x0A && **in != 0x0D) {
			break;
		}
		if(out) buffer_append(&buffer, *in, 1);
		(*in)++;
	}

	if(buffer.length != 0) {
		out->content = buffer.content;
		out->length = buffer.length;
	}

	return JSON_ERROR_OK;
}


enum json_error json_parser_scan_object(const char** in, const char* end, struct json_object* out) {

	assert(*in < end);

	if(**in != '{') return JSON_ERROR_OK;

	(*in)++;

	struct json_string* keys = NULL;
	struct json_value* values = NULL;
	unsigned int size = 0, length = 0;
//	struct whitespace* whitespaces = NULL;
//	unsigned int whitespaces_size = 0, whitespaces_length = 0;

	enum json_error error;

	const char* tmp_pos;
	struct whitespace whitespace = { .length = 0 };

	while(*in < end) {

		json_parser_scan_whitespace(in, end, &whitespace);
//		if(whitespace.length) {
//			whitespace.length = 0;
//		}

		if(*in >= end) goto unexpected_end;

		struct json_string key;
		tmp_pos = *in;
		error = json_parser_scan_string(in, end, &key);
		if(error) goto error;
		if(*in == tmp_pos) break;
		if(*in >= end) goto unexpected_end;

		json_parser_scan_whitespace(in, end, &whitespace);
//		if(whitespace.length) {
//			whitespace.length = 0;
//		}

		if(*in >= end) goto unexpected_end;

		if(**in != ':') {
			error = JSON_ERROR_UNEXPECTED_TOKEN;
			goto error;
		}

		(*in)++;
		if(*in >= end) goto unexpected_end;

		json_parser_scan_whitespace(in, end, &whitespace);
//		if(whitespace.length) {
//			whitespace.length = 0;
//		}

		if(*in >= end) goto unexpected_end;

		struct json_value value;
		tmp_pos = *in;
		error = json_parser_scan_value(in, end, &value);
		if(error) goto error;
		if(*in == tmp_pos) {
			error = JSON_ERROR_UNEXPECTED_TOKEN;
			goto error;
		}

		if(size < length + 1) {
			if(size == 0) {
				size = 1;
				keys = malloc(sizeof(struct json_string));
				values = malloc(sizeof(struct json_value));
			} else {
				size *= 2;
				keys = realloc(keys, sizeof(struct json_string) * size);
				values = realloc(values, sizeof(struct json_value) * size);
			}
		}

		keys[length] = key;
		values[length] = value;
		length++;

		if(*in >= end) goto unexpected_end;

		tmp_pos = *in;
		json_parser_scan_whitespace(in, end, &whitespace);
//		if(whitespace.length) {
//			whitespace.length = 0;
//		}

		if(*in >= end) goto unexpected_end;

		if(**in != ',') break;

		(*in)++;
	}

	if(*in >= end) goto unexpected_end;

	if(**in != '}') {
		error = JSON_ERROR_UNEXPECTED_TOKEN;
		goto error;
	}

	(*in)++;

	out->keys = keys;
	out->values = values;
	out->length = length;
//	out->whitespaces = whitespaces;
//	out->whitespaces_length = whitespaces_length;

	return JSON_ERROR_OK;

 unexpected_end:

 	error = JSON_ERROR_UNEXPECTED_END;

 error:

	free(keys);
	free(values);
//	free(whitespaces);

 	return error;
}


enum json_error json_parser_scan_array(const char** in, const char* end, struct json_array* out) {

	assert(*in < end);

	if(**in != '[') return JSON_ERROR_OK;

	(*in)++;

	struct json_value* values = NULL;
	unsigned int size = 0, length = 0;
//	struct whitespace* whitespaces = NULL;
//	unsigned int whitespaces_size = 0, whitespaces_length = 0;

	enum json_error error;

	const char* tmp_pos;
	struct whitespace whitespace = { .length = 0 };

	while(*in < end) {

		json_parser_scan_whitespace(in, end, &whitespace);
//		if(whitespace.length) {
//			whitespace.length = 0;
//		}

		if(*in >= end) goto unexpected_end;

		struct json_value value;
		tmp_pos = *in;
		error = json_parser_scan_value(in, end, &value);
		if(error) goto error;
		if(*in == tmp_pos) {
			if(length == 0) break;
			error = JSON_ERROR_UNEXPECTED_TOKEN;
			goto error;
		}

		if(size < length + 1) {
			if(size == 0) {
				size = 1;
				values = malloc(sizeof(struct json_value));
			} else {
				size *= 2;
				values = realloc(values, sizeof(struct json_value) * size);
			}
		}

		values[length] = value;
		length++;

		if(*in >= end) goto unexpected_end;

		tmp_pos = *in;
		json_parser_scan_whitespace(in, end, &whitespace);
//		if(whitespace.length) {
//			whitespace.length = 0;
//		}

		if(*in >= end) goto unexpected_end;

		if(**in != ',') break;

		(*in)++;
	}

	if(*in >= end) goto unexpected_end;

	if(**in != ']') {
		error = JSON_ERROR_UNEXPECTED_TOKEN;
		goto error;
	}

	(*in)++;
	out->values = values;
	out->length = length;
//	out->whitespaces = whitespaces;
//	out->whitespaces_length = whitespaces_length;

	return JSON_ERROR_OK;

 unexpected_end:

 	error = JSON_ERROR_UNEXPECTED_END;

 error:

	free(values);
//	free(whitespaces);

 	return error;

}


enum json_error json_parser_scan_string(const char** in, const char* end, struct json_string* out) {
	assert(*in < end);

	if(**in != '"') return JSON_ERROR_OK;

	struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };

	enum json_error error = 0;

	(*in)++;

	while(*in < end) {

		char c = **in;

		if(c == '"') break;

		if(c != '\\') {

			if(c <= 0x1f || c == 0x7f) { // disable control characters
				error = JSON_ERROR_UNEXPECTED_TOKEN;
				goto error;
			}

			buffer_append(&buffer, &c, 1);

		} else {

			(*in)++;

			if(*in >= end) goto unexpected_end;

			// early declaration
			uint16_t unicode_char;
			const char* uend = *in + 5;

			switch(**in) {
			default:
				error = JSON_ERROR_UNEXPECTED_TOKEN;
				goto error;
			case '"':
			case '\\':
			case '/':
				c = **in;
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'u':
				if(uend > end) goto unexpected_end;

				(*in)++;

				unicode_char = 0;

				while(1) {
					unsigned char c;
					if(**in >= '0' && **in <= '9') c= **in - '0';
					else if(**in >= 'a' && **in <= 'f') c = **in - 'a' + 10;
					else if(**in >= 'A' && **in <= 'F') c = **in - 'A' + 10;
					else {
						error = JSON_ERROR_UNEXPECTED_TOKEN;
						goto error;
					}
					unicode_char |= c;
					(*in)++;
					if(*in >= uend) break;
					unicode_char <<= 4;
				}

//				printf("Unicode in: %x\n", unicode_char);

				// FIXME: I guess this could be optimized/cleaned
				if(unicode_char < 0x80) {
//					printf("Bytes: %x\n", unicode_char);
					buffer_append_char(&buffer, (unicode_char & 0xff));
				} else if(unicode_char < 0x800) {
					unsigned char firstByte = 0xc0 | (0x1f & (unicode_char >> 6));
					unsigned char secondByte = 0x80 | (0x3f & unicode_char);
//					printf("Bytes: %x %x\n", firstByte, secondByte);
					buffer_append_char(&buffer, firstByte);
					buffer_append_char(&buffer, secondByte);
				} else {
					unsigned char firstByte = 0xe0 | (0x1f & (unicode_char >> 12));
					unsigned char secondByte = 0x80 | (0x3f & (unicode_char >> 6));
					unsigned char thirdByte = 0x80 | (0x3f & unicode_char);
//					printf("Bytes: %x %x %x\n", firstByte, secondByte, thirdByte);
					buffer_append_char(&buffer, firstByte);
					buffer_append_char(&buffer, secondByte);
					buffer_append_char(&buffer, thirdByte);
				}
				continue;
			}
			buffer_append(&buffer, &c, 1);
		}

		(*in)++;
	}

	if(*in >= end) goto unexpected_end;

	if(**in != '"') {
		error = JSON_ERROR_UNEXPECTED_TOKEN;
		goto error;
	}

	(*in)++;

	out->content = buffer.content;
	out->length = buffer.length;

	return JSON_ERROR_OK;

 unexpected_end:

 	error = JSON_ERROR_UNEXPECTED_END;

 error:

	free(buffer.content);
	return error;

}


enum json_error json_parser_scan_number(const char** in, const char* end, struct json_number* out) {

	assert(*in < end);

	if(**in != '-' && !(**in >= '0' && **in <= '9'))
		return JSON_ERROR_OK;

	struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };

	enum json_error error;

	if(**in == '-') {
		buffer_append(&buffer, *in, 1);
		(*in)++;
	}

	if(*in >= end) goto unexpected_end;

	if(**in == '0') {
		buffer_append(&buffer, *in, 1);
		(*in)++;
	} else if(**in >= '1' && **in <= '9') {
		while(*in < end) {
			if(**in < '0' || **in > '9') break;
			buffer_append(&buffer, *in, 1);
			(*in)++;
		}
	} else {
		error = JSON_ERROR_UNEXPECTED_TOKEN;
		goto error;
	}

	if(*in < end && **in == '.') {

		buffer_append(&buffer, *in ,1);
		(*in)++;

		if(*in >= end) goto unexpected_end;

		if(**in < '0' || **in > '9') {
			error = JSON_ERROR_UNEXPECTED_TOKEN;
			goto error;
		}

		while(*in < end) {
			if(**in < '0' || **in > '9') break;
			buffer_append(&buffer, *in, 1);
			(*in)++;
		}
	}

	if(*in < end && (**in == 'e' || **in == 'E')) {

		buffer_append(&buffer, *in, 1);
		(*in)++;
		if(*in >= end) goto unexpected_end;

		if(**in == '+' || **in == '-') {
			buffer_append(&buffer, *in, 1);
			(*in)++;

			if(*in >= end) goto unexpected_end;
		}

		if(**in < '0' || **in > '9') {
			error = JSON_ERROR_UNEXPECTED_TOKEN;
			goto error;
		}

		while(*in < end) {
			if(**in < '0' || **in > '9') break;
			buffer_append(&buffer, *in, 1);
			(*in)++;
		}
	}

	out->content = buffer.content;
	out->length = buffer.length;

	return JSON_ERROR_OK;

 unexpected_end:

	error = JSON_ERROR_UNEXPECTED_END;

 error:

	free(buffer.content);
	return error;
}


enum json_error json_parser_scan_boolean(const char** in, const char* end, struct json_boolean* out) {

	assert(*in < end);

	const char* s = *in;
	if(*in + 4 <= end && s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
		out->value = 1;
		*in += 4;
	} else if(*in + 5 <= end && s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
		out->value = 0;
		*in += 5;
	}
	return JSON_ERROR_OK;
}


enum json_error json_parser_scan_null(const char** in, const char* end) {

	assert(*in < end);

	const char* s = *in;
	if(*in + 4 <= end && s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l') {
		*in += 4;
	}
	return JSON_ERROR_OK;
}


enum json_error json_parser_scan_value(const char** in, const char* end, struct json_value* out) {

	assert(*in < end);

	const char* pos = *in;
	enum json_error error;

	if(error = json_parser_scan_string(in, end, &out->value.string)) return error;
	if(pos != *in) {
		out->type = JSON_TYPE_STRING;
		return JSON_ERROR_OK;
	}

	if(error = json_parser_scan_number(in, end, &out->value.number)) return error;
	if(pos != *in) {
		out->type = JSON_TYPE_NUMBER;
		return JSON_ERROR_OK;
	}

	if(error = json_parser_scan_object(in, end, &out->value.object)) return error;
	if(pos != *in) {
		out->type = JSON_TYPE_OBJECT;
		return JSON_ERROR_OK;
	}

	if(error = json_parser_scan_array(in, end, &out->value.array)) return error;
	if(pos != *in) {
		out->type = JSON_TYPE_ARRAY;
		return JSON_ERROR_OK;
	}

	if(error = json_parser_scan_boolean(in, end, &out->value.boolean)) return error;
	if(pos != *in) {
		out->type = JSON_TYPE_BOOLEAN;
		return JSON_ERROR_OK;
	}

	if(error = json_parser_scan_null(in, end)) return error;
	if(pos != *in) {
		out->type = JSON_TYPE_NULL;
		return JSON_ERROR_OK;
	}

	return JSON_ERROR_OK;
}


/********************/
/** JSON generator **/
/********************/


void json_encode_string(const unsigned char* in, size_t length, struct buffer* out);

void print_value(const struct json_value*, unsigned int);
void print_string(const struct json_string*);
void print_number(const struct json_number*);
void print_object(const struct json_object*, unsigned int);
void print_array(const struct json_array*, unsigned int);
void print_boolean(const struct json_boolean*);
void print_null();


void print_value(const struct json_value* value, unsigned int level) {
	switch(value->type) {
//	case JSON_TYPE_UNDEFINED:
//		printf("undefined");
//		break;
	case JSON_TYPE_STRING:
		print_string(&value->value.string);
		break;
	case JSON_TYPE_NUMBER:
		print_number(&value->value.number);
		break;
	case JSON_TYPE_OBJECT:
		print_object(&value->value.object, level);
		break;
	case JSON_TYPE_ARRAY:
		print_array(&value->value.array, level);
		break;
	case JSON_TYPE_BOOLEAN:
		print_boolean(&value->value.boolean);
		break;
	case JSON_TYPE_NULL:
		print_null();
		break;
	default:
		assert(0);
	}
}


void print_string(const struct json_string* string) {
	struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };
	json_encode_string((unsigned char*)string->content, string->length, &buffer);
	printf("\"%.*s\"", (unsigned int)buffer.length, buffer.content);
}


void print_number(const struct json_number* number) {
	printf("%.*s", (unsigned int)number->length, number->content);
}


void print_object(const struct json_object* object, unsigned int level) {
	printf("{\n");

	level++;

	int i, ii;
	for(i = 0; i < object->length; i++) {
		for(ii = 0; ii < level; ii++) printf("\t");
		print_string(&object->keys[i]);
		printf(" : ");
		print_value(&object->values[i], level);
		if(i != object->length - 1) printf(",");
		printf("\n");
	}

	for(ii = 0; ii < level - 1; ii++) printf("\t");

	printf("}");
}


void print_array(const struct json_array* array, unsigned int level) {
	printf("[\n");

	level++;

	int i, ii;
	for(i = 0; i < array->length; i++) {
		for(ii = 0; ii < level; ii++) printf("\t");
		print_value(&array->values[i], level);
		if(i+1 < array->length) printf(",");
		printf("\n");
	}

	for(ii = 0; ii < level - 1; ii++) printf("\t");

	printf("]");
}


void print_boolean(const struct json_boolean* boolean) {
	printf(boolean->value ? "true" : "false");
}


void print_null() {
	printf("null");
}


/****************/
/** JSON utils **/
/****************/


struct path {
	struct json_string* components;
	size_t length;
};


// FIXME: hacking with const
int json_resolve_path(const struct json_value*, const struct path*, const struct json_value** out);
struct json_value* json_object_resolve(const struct json_object*, const struct json_string*);
int json_string_to_index(const struct json_string*, size_t*);
void json_object_set(struct json_object*, const struct json_string*, const struct json_value*, struct json_value*);
void json_array_set(struct json_array*, size_t, const struct json_value*, struct json_value*);
void json_encode_string(const unsigned char*, size_t, struct buffer*);


int json_resolve_path(const struct json_value* in, const struct path* path, const struct json_value** out) {
	int i;
	for(i = 0; i < path->length; i++) {

		const struct json_string* component = &path->components[i];

		if(in->type == JSON_TYPE_OBJECT) {
			in = json_object_resolve(&in->value.object, component);
			if(in == NULL) break;
		} else if(in->type == JSON_TYPE_ARRAY) {

			// because I do not trust standard number parsers and my use case is simplistic anyway
			size_t index = 0;

			if(json_string_to_index(component, &index) || index >= in->value.array.length) {
				in = NULL;
				break;
			}

			in = &in->value.array.values[index];
		} else {
			in = NULL;
			break;
		}
	}

	*out = in;

	return i;
}


struct json_value* json_object_resolve(const struct json_object* object, const struct json_string* key) {
	struct json_value* property_value = NULL;
	size_t ii;
	for(ii = 0; ii < object->length; ii++) {
		if(key->length == object->keys[ii].length && strncmp(object->keys[ii].content, key->content, key->length) == 0) {
			property_value = &object->values[ii];
		}
	}

	return property_value;
}


int json_string_to_index(const struct json_string* string, size_t* out) {
	size_t index = 0;
	size_t ii;
	for(ii = 0; ii < string->length; ii++) {
		char c = string->content[ii];
		if(c < '0' || c > '9') {
			return -1;
		}

		int n =	c - '0';

		// overflow chack
		if(index > SIZE_MAX / 10 || (index *= 10) > SIZE_MAX - n) {
			fprintf(stderr, "Overflow\n");
			return -1;
		}

		index += n;
	}

	*out = index;
	return 0;
}


void json_object_set(struct json_object* object, const struct json_string* key, const struct json_value* value, struct json_value* old_value) {
	struct json_value* v = json_object_resolve(object, key);
	if(v) {
		size_t index = v - object->values;

//		if(old_key) *old_key = object->keys[index];			// FIXME: key will be dangling
		if(old_value) *old_value = object->values[index];

		if(value->type == JSON_TYPE_UNDEFINED) {
			object->length--;
			memmove(&object->keys[index], &object->keys[index+1], (object->length - index) * sizeof(struct json_string));
			memmove(&object->values[index], &object->values[index+1], (object->length - index) * sizeof(struct json_value));
		} else {
			object->keys[index] = *key;
			object->values[index] = *value;
		}
	} else if(value->type != JSON_TYPE_UNDEFINED) {
		object->keys = realloc(object->keys, (object->length + 1) * sizeof(struct json_string));
		object->values = realloc(object->values, (object->length + 1) * sizeof(struct json_value));

		object->keys[object->length] = *key;
		object->values[object->length] = *value;

		object->length++;
		if(old_value) old_value->type = JSON_TYPE_UNDEFINED;
	}
}


void json_array_set(struct json_array* array, size_t index, const struct json_value* value, struct json_value* old_value) {
	if(index >= array->length) {
		// fill gap
		array->values = realloc(array->values, (index + 1) * sizeof(struct json_value));
		int i;
		for(i = array->length; i < index; i++) {
			array->values[i].type = JSON_TYPE_NULL;
		}
		array->length = index + 1;

		if(old_value) old_value->type = JSON_TYPE_UNDEFINED;
	} else {
		if(old_value) *old_value = array->values[index];
	}
	array->values[index] = *value;
	if(value->type == JSON_TYPE_UNDEFINED) array->values[index].type = JSON_TYPE_NULL;
}


void json_encode_string(const unsigned char* in, size_t length, struct buffer* out) {
	struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };

	uint16_t unicode_char = 0;
	const unsigned char* end = in + length;

	while(in < end) {
		switch(*in) {
		case '"':
			buffer_append(&buffer, "\\\"", 2);
			break;
		case '\\':
			buffer_append(&buffer, "\\\\", 2);
			break;
		case '/':
			buffer_append(&buffer, "\\/", 2);
			break;
		case '\b':
			buffer_append(&buffer, "\\b", 2);
			break;
		case '\f':
			buffer_append(&buffer, "\\f", 2);
			break;
		case '\n':
			buffer_append(&buffer, "\\n", 2);
			break;
		case '\r':
			buffer_append(&buffer, "\\r", 2);
			break;
		case '\t':
			buffer_append(&buffer, "\\t", 2);
			break;
		default:
			// FIXME: this could be optimized/cleaned?
			if(*in < 0x80) {
				buffer_append(&buffer, (const char*)in, 1);
			} else {
				if((*in >> 5) == 0x06) {
					// 2-byte unicode

//					fprintf(stderr, "2-byte unicode: %x", *in);

					unicode_char = (*in & 0x1f) << 6;

					if(in + 1 < end) {
						in++;
//						fprintf(stderr, " %x", *in);
						unicode_char |= *in & 0x3f;
					}
//					fprintf(stderr, "\n");
				} else if((*in >> 4) == 0x0e) {
					// 3-byte unicode

//					fprintf(stderr, "3-byte unicode: %x", *in);

					unicode_char = (*in & 0x0f) << 12;

					if(in + 1 < end) {
						in++;
//						fprintf(stderr, " %x", *in);
						unicode_char |= (*in & 0x3f) << 6;
					}

					if(in + 1 < end) {
						in++;
//						fprintf(stderr, " %x", *in);
						unicode_char |= *in & 0x3f;
					}
//					fprintf(stderr, "\n");
				} else {
//					fprintf(stderr, "%x %x\n", *in, *(in + 1));
					exit(1);
				}

				char b[6] = "\\u";
				b[2] = (unicode_char & 0xf000) >> 12;
				b[3] = (unicode_char & 0x0f00) >> 8;
				b[4] = (unicode_char & 0x00f0) >> 4;
				b[5] = (unicode_char & 0x000f) >> 0;
//				fprintf(stderr, "Unicode: %x%x%x%x\n", b[2], b[3], b[4], b[5]);
				b[2] += b[2] >= 0xa ? 'a' - 0xa : '0';
				b[3] += b[3] >= 0xa ? 'a' - 0xa : '0';
				b[4] += b[4] >= 0xa ? 'a' - 0xa : '0';
				b[5] += b[5] >= 0xa ? 'a' - 0xa : '0';
				buffer_append(&buffer, b, 6);
			}
		}
		in++;
	}
	*out = buffer;
}


/**********/
/** main **/
/**********/


enum op {
	OP_UNKNOWN,
	// make sure that input contains 1 and only 1 valid JSON value
	OP_CHECK,
	// print type name of input value
	OP_TYPE,
	// get element of array or property of object
	OP_GET,
	// get list of keys in object
	OP_KEYS,
	// set element of array or property of object
	OP_SET,
	// add element to array
	OP_SPLICE,
	// decode JSON-encoded string string to UTF-8 encoded string
	OP_DECODE_STRING,
	// escape string characters
	OP_ENCODE_STRING,
	// escape periods and backslashes in path component
	OP_ENCODE_KEY,
};


void print_usage(const char* program_name) {
	// TODO: write comprehensive usage text
	printf("Usage: %s ACTION OPTIONS\n", program_name);
}


int parse_path(const char* in, struct path* path) {

	struct json_string* components = malloc(1 * sizeof(struct json_string));
	size_t components_size = 1;
	size_t components_length = 0;

	struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };

	while(*in != '\0') {
		if(*in == '.') {
			if(components_length >= components_size) {
				components = realloc(components, (components_size *= 2) * sizeof(struct json_string));
			}

			components[components_length].content = buffer.content;
			components[components_length].length = buffer.length;
			components_length++;

			buffer.content = NULL;
			buffer.length = buffer.size = 0;
		} else {
			if(*in == '\\') {
				in++;
				if(*in != '.' && *in != '\\') goto error;
			}

			buffer_append(&buffer, in, 1);
		}
		in++;
	}

	if(components_length >= components_size) {
		components = realloc(components, (components_size *= 2) * sizeof(struct json_string));
	}

	components[components_length].content = buffer.content;
	components[components_length].length  = buffer.length;

	path->components = components;
	path->length = components_length + 1;

	return 0;

 error:

	free(buffer.content);

	while(components_length--) {
		free(components[components_length].content);
	}

	free(components);

	return -1;
}


int parse_input(const char** start, const char* end, struct json_value** out, size_t* out_length) {
	struct json_value* values = malloc(2 * sizeof(struct json_value));
	size_t size = 2, length = 0;
	size_t max = *out_length ? *out_length : SIZE_MAX;

	while(*start < end && max--) {

		json_parser_scan_whitespace(start, end, NULL);

		if(*start >= end) break;

		const char* tmp_pos = *start;
		struct json_value value;
		enum json_error error = json_parser_scan_value(start, end, &value);

		if(error || *start == tmp_pos) goto error;

		if(length >= size) values = realloc(values, (size *= 2) * sizeof(struct json_value));
		values[length++] = value;
	}

	*out = values;
	*out_length = length;

	return 0;

 error:

	free(values);

	return -1;
}


int main(int argc, const char* const* argv) {

	struct buffer stdin_buffer = { .content = malloc(4), .length = 0, .size = 4 };
	enum op op = OP_UNKNOWN;


	if(argc < 2) {
		print_usage(argv[0]);
		exit(1);
	}


	if(strcmp(argv[1], "check") == 0) op = OP_CHECK;
	else if(strcmp(argv[1], "type") == 0) op = OP_TYPE;
	else if(strcmp(argv[1], "get") == 0) op = OP_GET;
	else if(strcmp(argv[1], "keys") == 0) op = OP_KEYS;
	else if(strcmp(argv[1], "set") == 0) op = OP_SET;
	else if(strcmp(argv[1], "splice") == 0) op = OP_SPLICE;
	else if(strcmp(argv[1], "decode-string") == 0) op = OP_DECODE_STRING;
	else if(strcmp(argv[1], "encode-string") == 0) op = OP_ENCODE_STRING;
	else if(strcmp(argv[1], "encode-key") == 0) op = OP_ENCODE_KEY;
	else {
		fprintf(stderr, "%s: Invalid action %s\n", argv[0], argv[1]);
	}


	// read stdin for these actions and parse as JSON if needed
	if(op == OP_CHECK || op == OP_TYPE || op == OP_GET || op == OP_KEYS || // read operations
	   op == OP_SET || op == OP_SPLICE || // write operation
	   op == OP_DECODE_STRING || op == OP_ENCODE_STRING /* || op == OP_ENCODE_KEY */ // utils
	   ) {

		int r;
		while(r = read(0, stdin_buffer.content + stdin_buffer.length, stdin_buffer.size - stdin_buffer.length)) {
			if(r < 0) {
				fprintf(stderr, "%s: Error reading stdin: (%d) %s\n", argv[0], errno, strerror(errno));
				exit(1);
			}

			stdin_buffer.length += r;
			if(stdin_buffer.length >= stdin_buffer.size)
				stdin_buffer.content = realloc(stdin_buffer.content, stdin_buffer.size *= 2);
		}
	}


	if(op == OP_CHECK) {
		struct json_value* json_in;
		size_t length = 2;

		const char* start = stdin_buffer.content;
		const char* end = start + stdin_buffer.length;
		if(parse_input(&start, end, &json_in, &length) || length != 1) {
			printf("ERROR");
		}
	}


	if(op == OP_TYPE) {
		struct json_value* json_in;
		size_t length = 1;

		const char* start = stdin_buffer.content;
		if(parse_input(&start, start + stdin_buffer.length, &json_in, &length) || length < 1) {
			fprintf(stderr, "%s: Invalid input\n", argv[0]);
			exit(1);
		}

		switch(json_in->type) {
		case JSON_TYPE_OBJECT:
			printf("object");
			break;
		case JSON_TYPE_ARRAY:
			printf("array");
			break;
		case JSON_TYPE_STRING:
			printf("string");
			break;
		case JSON_TYPE_NUMBER:
			printf("number");
			break;
		case JSON_TYPE_BOOLEAN:
			printf("boolean");
			break;
		case JSON_TYPE_NULL:
			printf("null");
			break;
		default:
			assert(0);
		}
	}


	if(op == OP_GET) {

		struct path path;
		struct json_value* json_in;
		size_t length = 1;

		if(argc < 3) {
			fprintf(stderr, "Usage: %s %s pathname\n", argv[0], argv[1]);
			exit(1);
		}

		if(parse_path(argv[2], &path)) {
			fprintf(stderr, "%s: Invalid path %s for action %s\n", argv[0], argv[2], argv[1]);
			exit(1);
		}

		const char* start = stdin_buffer.content;
		if(parse_input(&start, start + stdin_buffer.length, &json_in, &length) || length < 1) {
			fprintf(stderr, "%s: Invalid input\n", argv[0]);
			exit(1);
		}

		const struct json_value* resolved_value;
		int r = json_resolve_path(json_in, &path, &resolved_value);

		if(r == path.length) {
			print_value(resolved_value, 0);
		}
	}


	if(op == OP_SET) {

		struct path path;
		struct json_value* json_in;
		struct json_value* value;
		size_t length = 2;

		if(argc < 3) {
			fprintf(stderr, "Usage: %s %s pathname\n", argv[0], argv[1]);
			exit(1);
		}

		if(parse_path(argv[2], &path)) {
			fprintf(stderr, "%s: Invalid path %s for action %s\n", argv[0], argv[2], argv[1]);
			exit(1);
		}

		const char* start = stdin_buffer.content;
		if(parse_input(&start, start + stdin_buffer.length, &json_in, &length) || length < 1) {
			fprintf(stderr, "%s: Invalid input\n", argv[0]);
			exit(1);
		}

		if(length < 2) {
			value = malloc(sizeof(struct json_value));
			value->type = JSON_TYPE_UNDEFINED;
		} else {
			value = &json_in[1];
		}

		struct json_value* resolved_value;

		path.length--;

		if(json_resolve_path(json_in, &path, (const struct json_value**)&resolved_value) == path.length) {

			if(resolved_value->type == JSON_TYPE_OBJECT) {
				json_object_set(&resolved_value->value.object, &path.components[path.length], value, NULL);
			}

			if(resolved_value->type == JSON_TYPE_ARRAY) {
				size_t index;
				if(!json_string_to_index(&path.components[path.length], &index)) {
					json_array_set(&resolved_value->value.array, index, value, NULL);
				}
			}
		}

		print_value(json_in, 0);
	}


	if(op == OP_KEYS) {

		struct json_value* json_in;
		size_t length = 1;

		const char* start = stdin_buffer.content;
		if(parse_input(&start, start + stdin_buffer.length, &json_in, &length) || length < 1) {
			fprintf(stderr, "%s: Invalid input\n", argv[0]);
			exit(1);
		}

		if(json_in->type != JSON_TYPE_OBJECT) {
			fprintf(stderr, "%s: Expected JSON object as input\n", argv[0]);
			exit(1);
		}

		struct json_object* object = &json_in->value.object;

		int i;
		for(i = 0; i < object->length; i++) {
			print_string(&object->keys[i]);
			printf("\n");
		}
	}


	if(op == OP_SPLICE) {

		struct json_value* json_in;
		size_t length = 0;

		size_t index = SIZE_MAX, count = 0;

		errno = 0;
		if(argc >= 3) {
			const char* end = argv[2];
			index = strtoumax(argv[2], (char**)&end, 0);
			if(argv[2][0] == '-' || *end != '\0' || end == argv[2] || errno != 0) {
				fprintf(stderr, "%s: Invalid index\n", argv[0]);
				exit(1);
			}
		}

		if(argc >= 4) {
			const char* end = argv[3];
			count = strtoumax(argv[3], (char**)&end, 0);
			if(argv[3][0] == '-' || *end != '\0' || end == argv[3] || errno != 0) {
				fprintf(stderr, "%s: Invalid element count\n", argv[0]);
				exit(1);
			}
		}

		const char* start = stdin_buffer.content;
		if(parse_input(&start, start + stdin_buffer.length, &json_in, &length) || length < 1) {
			fprintf(stderr, "%s: Invalid input\n", argv[0]);
			exit(1);
		}

		if(json_in->type != JSON_TYPE_ARRAY) {
			fprintf(stderr, "%s: Expected JSON array as input\n", argv[0]);
			exit(1);
		}

		length--;

		struct json_array* array = &json_in->value.array;

		if(index >= array->length) {
			count = 0;
			index = array->length;
		} else if(index + count >= array->length) {
			count = array->length - index;
		}

		size_t new_size = array->length + length - count;
		if(array->length < new_size) {
			array->values = realloc(array->values, new_size * sizeof(struct json_value));
		}

		size_t shift_dst = index + length;
		size_t shift_src = index + count;
		if(shift_src != shift_dst) {
			size_t shift_count = array->length - shift_src;
			memmove(&array->values[shift_dst], &array->values[shift_src], shift_count * sizeof(struct json_value));
		}

		if(length) {
			memcpy(&array->values[index], &json_in[1], length * sizeof(struct json_value));
		}

		array->length = new_size;

		print_array(array, 0);
	}


	if(op == OP_DECODE_STRING) {

		struct json_value* json_in;
		size_t length = 1;
		const char* start = stdin_buffer.content;
		if(parse_input(&start, start + stdin_buffer.length, &json_in, &length) || length < 1) {
			fprintf(stderr, "%s: Invalid input\n", argv[0]);
			exit(1);
		}

		if(json_in->type != JSON_TYPE_STRING) {
			fprintf(stderr, "%s: Expected JSON string as input\n", argv[0]);
			exit(1);
		}

		printf("%.*s", (unsigned int)json_in->value.string.length, json_in->value.string.content);
	}


	if(op == OP_ENCODE_STRING) {
		struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };
		json_encode_string((unsigned char*)stdin_buffer.content, stdin_buffer.length, &buffer);

		printf("%.*s", (unsigned int)buffer.length, buffer.content);
	}


	if(op == OP_ENCODE_KEY) {
		if(argc < 3) {
			fprintf(stderr, "Usage %s %s pathcomponent Missing argument action\n", argv[0], argv[1]);
			exit(1);
		}

		const char* arg = argv[2];

		struct buffer buffer = { .content = NULL, .length = 0, .size = 0 };
		while(*arg != '\0') {
			if(*arg == '.' || *arg == '\\')
				buffer_append(&buffer, "\\", 1);
			buffer_append(&buffer, arg, 1);
			arg++;
		}
		printf("%.*s", (unsigned int)buffer.length, buffer.content);
	}

	return 0;
}
