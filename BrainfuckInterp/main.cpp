#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stack>

#ifndef CUSTOM_CELL_SIZE	// cell size in bits, valid values are 8, 16, 32, 64
#define CELL_SIZE 8
#endif

#ifndef CUSTOM_CELL_WRAP_BEHAVIOR	// wrapping for cell, 1 for wrap, 0 for not wrap.
#define CELL_WRAP 1
#endif

using size_t = decltype(sizeof(void*));
using funcptr = int(*)(int, size_t&, char*);

#if CELL_SIZE == 8
#if CELL_WRAP == 1
using cell_type = int8_t;
#define PRINT_C "%c"
#else
using cell_type = uint8_t;
#define PRINT_C "%hhu"
#endif
#elif CELL_SIZE == 16
#if CELL_WRAP == 1
using cell_type = int16_t;
#define PRINT_C "%hd"
#else
using cell_type = uint16_t;
#define PRINT_C "%hu"
#endif
#elif CELL_SIZE == 32
#if CELL_WRAP == 1
using cell_type = int32_t;
#define PRINT_C "%d"
#else
using cell_type = uint32_t;
#define PRINT_C "%u"
#endif
#elif CELL_SIZE == 64
#if CELL_WRAP == 1
using cell_type = int64_t;
#define PRINT_C "%lld"
#else
using cell_type = uint64_t;
#define PRINT_C "%lld"
#endif
#else
#error "Cell sizes supported are 8, 16, 32, 64 bytes"
#endif

// recommended size of buffer_size apparently
// from esolangs.org
constexpr size_t buffer_size = 30'000;

// global state
static cell_type execution_buffer[buffer_size] = { 0 };
static std::stack<size_t> parens{};

struct fn_pair {
	char c;
	funcptr fn;
};

int action_increment_index(int current_index, size_t&, char*) {
	// >
	return current_index + 1;
}
int action_decrement_index(int current_index, size_t&, char*) {
	// <
	return current_index - 1;
}
int action_increment_cell(int current_index, size_t&, char*) {
	// +
	++execution_buffer[current_index];
	return current_index;
}
int action_decrement_cell(int current_index, size_t&, char*) {
	// -
	--execution_buffer[current_index];
	return current_index;
}
int action_print_cell(int current_index, size_t&, char*) {
	// .
	if constexpr (CELL_SIZE == 8) {
		if constexpr ('\n' == 10) {
			putchar(execution_buffer[current_index]);
		}
		else {
			putchar(execution_buffer[current_index] == 10 ? '\n' : execution_buffer[current_index]);
		}
	}
	else {
		// for cell sizes that are not 8 it doesn't make sense to print a char
		if (cell_type{ '\n' } == execution_buffer[current_index]) {
			putchar('\n');
		}
		else {
			printf(PRINT_C, execution_buffer[current_index]);
		}
	}
	return current_index;
}

int action_input_cell(int current_index, size_t&, char*) {
	// ,
	// for input we always read a char regardless
	if constexpr ('\n' == 10) {
		execution_buffer[current_index] = static_cast<cell_type>(getchar());
	}
	else {
		char nc = getchar();
		execution_buffer[current_index] = (nc == '\n') ? 10 : nc;
	}
	return current_index;
}

int action_loop_open(int current_index, size_t& text_index, char* text_buffer) {
	// [
	parens.push(text_index);
	return current_index;
}
int action_loop_close(int current_index, size_t& text_index, char*) {
	// ]
	if (parens.empty()) {
		printf("Unbalanced []\n");
		exit(EXIT_FAILURE);
	}
	if (execution_buffer[current_index] != 0) {
		text_index = parens.top();
	}
	else {
		parens.pop();
	}
	return current_index;
}

int main(int argc, char* argv[]) {
	static constexpr fn_pair keychars[] = {
		{'>', action_increment_index},
		{'<', action_decrement_index},
		{'+', action_increment_cell},
		{'-', action_decrement_cell},
		{'.', action_print_cell},
		{',', action_input_cell},
		{'[', action_loop_open},
		{']', action_loop_close}
	};

	auto check_char = [](char c) -> funcptr {
		for (const auto& [kc, fn] : keychars) {
			if (c == kc) return fn;
		}
		return nullptr;
	};

	char filename_buffer[FILENAME_MAX + 1] = { 0 };
	int current_buffer_index = 0;
	size_t text_index = 0;
	if (argc > 1) {
		strncpy_s(filename_buffer, argv[1], FILENAME_MAX);
		filename_buffer[FILENAME_MAX] = '\0';
	}
	else {
		printf("Insert a filename here:\n");
		fgets(filename_buffer, FILENAME_MAX, stdin);
		filename_buffer[strlen(filename_buffer) - 1] = '\0';		// get rid of '\n'
	}
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, filename_buffer, "r");
	if (err) {
		printf("Couldn't open file %s\n", filename_buffer);
		return EXIT_FAILURE;
	}
	fseek(fp, 0, SEEK_END);
	auto len = static_cast<size_t>(ftell(fp));
	char* text = new char[len];
	fseek(fp, 0, SEEK_SET);
	text[fread(text, sizeof(char), len, fp)] = '\0';
	fclose(fp);
	while (text_index < len && text[text_index] != '\0') {
		char c = text[text_index++];
		auto* f = check_char(c);
		if (f) {
			current_buffer_index = f(current_buffer_index, text_index, text);
		}
		// wrapping behavior
		if (current_buffer_index < 0)
			current_buffer_index = buffer_size;
		else if (current_buffer_index > buffer_size) {
			current_buffer_index = 0;
		}
	}
	delete[] text;
	return EXIT_SUCCESS;
}