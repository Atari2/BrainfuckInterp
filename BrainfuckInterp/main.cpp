#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

#ifndef CUSTOM_CELL_SIZE	// cell size in bits, valid values are 8, 16, 32, 64
#define CELL_SIZE 8
#endif

#ifndef CUSTOM_CELL_WRAP_BEHAVIOR	// wrapping for cell, 1 for wrap, 0 for not wrap.
#define CELL_WRAP 1
#endif

using size_t = decltype(sizeof(void*));
using funcptr = int(*)(int);

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

struct fn_pair {
	char c;
	funcptr fn;
};

class parens_stack_t {
	size_t* m_stack_ptr = nullptr;
	size_t m_capacity = 0;
	size_t m_size = 0;
	static constexpr inline size_t m_base_capacity = 64;
	bool verify_capacity() {
		return m_size < m_capacity;
	}
	void grow() {
		if (m_capacity == 0) {
			m_capacity = m_base_capacity;
			m_stack_ptr = static_cast<size_t*>(malloc(m_base_capacity * sizeof(size_t)));
		}
		else {
			m_capacity *= 2;
			size_t* new_mem = static_cast<size_t*>(realloc(m_stack_ptr, m_capacity));
			if (new_mem == nullptr) {
				free(m_stack_ptr);
				printf("Error while allocating memory. Exiting");
				exit(EXIT_FAILURE);
			}
			m_stack_ptr = new_mem;
		}
	}
	void ensure_capacity() {
		if (!verify_capacity()) grow();
	}
public:
	constexpr parens_stack_t() = default;
	bool empty() const {
		return m_size == 0;
	}
	size_t top() const {
		return m_stack_ptr[m_size - 1];
	}
	void pop() {
		if (m_size == 0) {
			printf("Trying to pop from empty stack. Exiting.");
			exit(EXIT_FAILURE);
		}
		m_size--;
	}
	void push(size_t val) {
		ensure_capacity();
		m_stack_ptr[m_size++] = val;
	}
	~parens_stack_t() {
		if (m_capacity > 0 && m_stack_ptr) free(m_stack_ptr);
	}
};

class state {
	static inline cell_type m_execution_buffer[buffer_size] = { 0 };
	static inline size_t m_text_index = 0;
	static inline char* m_text_buffer = nullptr;
	static inline parens_stack_t m_parens_stack{};

public:
	static auto* execution_buffer() { return m_execution_buffer; }
	static auto& text_index() { return m_text_index; }
	static auto& parens() { return m_parens_stack; }
	static auto* text_buffer() { return m_text_buffer; }
	static void allocate_text_buffer(size_t size) {
		m_text_buffer = static_cast<char*>(malloc(sizeof(char) * size));
	}
	static void deallocate_text_buffer() {
		free(m_text_buffer);
	}
};

// global state

int action_increment_index(int current_index) {
	// >
	return current_index + 1;
}
int action_decrement_index(int current_index) {
	// <
	return current_index - 1;
}
int action_increment_cell(int current_index) {
	// +
	++state::execution_buffer()[current_index];
	return current_index;
}
int action_decrement_cell(int current_index) {
	// -
	--state::execution_buffer()[current_index];
	return current_index;
}
int action_print_cell(int current_index) {
	// .
	if constexpr (CELL_SIZE == 8) {
		if constexpr ('\n' == 10) {
			putchar(state::execution_buffer()[current_index]);
		}
		else {
			putchar(state::execution_buffer()[current_index] == 10 ? '\n' : state::execution_buffer()[current_index]);
		}
	}
	else {
		// for cell sizes that are not 8 it doesn't make sense to print a char
		if (cell_type{ '\n' } == state::execution_buffer()[current_index]) {
			putchar('\n');
		}
		else {
			printf(PRINT_C, state::execution_buffer()[current_index]);
		}
	}
	return current_index;
}

int action_input_cell(int current_index) {
	// ,
	// for input we always read a char regardless
	if constexpr ('\n' == 10) {
		state::execution_buffer()[current_index] = static_cast<cell_type>(getchar());
	}
	else {
		char nc = getchar();
		state::execution_buffer()[current_index] = (nc == '\n') ? 10 : nc;
	}
	return current_index;
}

int action_loop_open(int current_index) {
	// [
	state::parens().push(state::text_index());
	return current_index;
}
int action_loop_close(int current_index) {
	// ]
	if (state::parens().empty()) {
		printf("Unbalanced []\n");
		exit(EXIT_FAILURE);
	}
	if (state::execution_buffer()[current_index] != 0) {
		state::text_index() = state::parens().top();
	}
	else {
		state::parens().pop();
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
	state::allocate_text_buffer(len + 1);
	fseek(fp, 0, SEEK_SET);
	state::text_buffer()[fread(state::text_buffer(), sizeof(char), len, fp)] = '\0';
	fclose(fp);
	while (state::text_index() < len && state::text_buffer()[state::text_index()] != '\0') {
		char c = state::text_buffer()[state::text_index()++];
		auto* f = check_char(c);
		if (f) {
			current_buffer_index = f(current_buffer_index);
		}
		// wrapping behavior
		if (current_buffer_index < 0)
			current_buffer_index = buffer_size - 1;
		else if (current_buffer_index >= buffer_size) {
			current_buffer_index = 0;
		}
	}
	state::deallocate_text_buffer();
	return EXIT_SUCCESS;
}