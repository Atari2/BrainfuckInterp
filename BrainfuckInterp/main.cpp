#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stack>

#ifndef CUSTOM_CELL_SIZE
#define CELL_SIZE 8
#endif

#ifndef CUSTOM_CELL_WRAP_BEHAVIOR
#define CELL_WRAP 1
#endif

using size_t = decltype(sizeof(void*));

#if CELL_SIZE == 8
#if CELL_WRAP == 1
using cell_type = int8_t;
#else
using cell_type = uint8_t;
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

template <typename T, size_t N>
constexpr size_t sizeof_array(const T(&val)[N]) {
	return N;
}

int do_action(char c, int current_index, cell_type* execution_buffer, size_t& text_index, char* text_buffer, std::stack<size_t>& parens) {
	switch (c) {
	case '>':
		return current_index + 1;
	case '<':
		return current_index - 1;
	case '+':
		++execution_buffer[current_index];
		return current_index;
	case '-':
		--execution_buffer[current_index];
		return current_index;
	case '.':
#if CELL_SIZE == 8
#if '\n' == 10
		putchar(execution_buffer[current_index]);
#else
		putchar(execution_buffer[current_index] == 10 ? '\n' : execution_buffer[current_index]);
#endif
#else
		// for cell sizes that are not 8 it doesn't make sense to print a char
		if (cell_type{ '\n' } == execution_buffer[current_index]) {
			putchar('\n');
		}
		else {
			printf(PRINT_C, execution_buffer[current_index]);
		}
#endif
		return current_index;
	case ',': {
		// for input we always read a char regardless
#if '\n' == 10
		execution_buffer[current_index] = static_cast<cell_type>(getchar());
#else
		char nc = getchar();
		execution_buffer[current_index] = (nc == '\n') ? 10 : nc;
#endif
		return current_index;
	}
	case '[':
		parens.push(text_index);
		return current_index;
	case ']': {
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
	default:
		return current_index;
	}
}

int main(int argc, char* argv[]) {
	constexpr char keychars[] = { '>', '<', '+', '-', '.', ',', '[', ']' };

	std::stack<size_t> parens{};

	using funcptr = decltype(&do_action);

	auto check_char = [&keychars](char c) -> funcptr {
		for (size_t i = 0; i < sizeof_array(keychars); i++) {
			if (c == keychars[i]) return &do_action;
		}
		return nullptr;
	};
	char filename_buffer[FILENAME_MAX + 1] = { 0 };
	// recommended size of buffer_size apparently
	// from esolangs.org
	constexpr size_t buffer_size = 30'000;
	cell_type* execution_buffer = new cell_type[buffer_size];
	memset(execution_buffer, 0, buffer_size);
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
	while (text_index < len && text[text_index] != '\0') {
		char c = text[text_index++];
		auto* f = check_char(c);
		if (f) {
			current_buffer_index = f(c, current_buffer_index, execution_buffer, text_index, text, parens);
		}
		// wrapping behavior
		if (current_buffer_index < 0)
			current_buffer_index = buffer_size;
		else if (current_buffer_index > buffer_size) {
			current_buffer_index = 0;
		}
	}
	return EXIT_SUCCESS;
}