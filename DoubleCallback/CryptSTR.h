#include <tuple>
#include <iostream>
#include <array>
#include <utility>

template <typename value_type, size_t length>
class xorstr
{
private:
#define CountOf(align, size) ((((size + align - 1) / align) * align) / align)
	static constexpr auto count = CountOf(8, sizeof(value_type[length]));
	uint64_t data[count];

	__forceinline constexpr const uint64_t crypt(bool gg, const value_type* c, size_t i)
	{
		if (gg) {
			constexpr volatile uint64_t key = (
				(uint64_t)((__TIME__[0] * __TIME__[6]) + __TIME__[1]) << 56 |
				(uint64_t)((__TIME__[1] * __TIME__[6]) + __TIME__[0]) << 48 |
				(uint64_t)((__TIME__[3] * __TIME__[6]) + __TIME__[4]) << 40 |
				(uint64_t)((__TIME__[4] * __TIME__[6]) + __TIME__[3]) << 32 |
				(uint64_t)((__TIME__[0] * __TIME__[7]) + __TIME__[1]) << 24 |
				(uint64_t)((__TIME__[1] * __TIME__[7]) + __TIME__[0]) << 16 |
				(uint64_t)((__TIME__[3] * __TIME__[7]) + __TIME__[4]) << 8 |
				(uint64_t)((__TIME__[4] * __TIME__[7]) + __TIME__[3]) << 0);

			auto ptr = (const uint8_t*)c + (i * 8);
			uint8_t ret[8] = {
				((i * 8) + 0) < size() ? ptr[0] : (uint8_t)0,
				((i * 8) + 1) < size() ? ptr[1] : (uint8_t)0,
				((i * 8) + 2) < size() ? ptr[2] : (uint8_t)0,
				((i * 8) + 3) < size() ? ptr[3] : (uint8_t)0,
				((i * 8) + 4) < size() ? ptr[4] : (uint8_t)0,
				((i * 8) + 5) < size() ? ptr[5] : (uint8_t)0,
				((i * 8) + 6) < size() ? ptr[6] : (uint8_t)0,
				((i * 8) + 7) < size() ? ptr[7] : (uint8_t)0
			}; return *(uint64_t*)&ret ^ key;
		}

		else {
			constexpr uint64_t key = (
				(uint64_t)((__TIME__[0] * __TIME__[6]) + __TIME__[1]) << 56 |
				(uint64_t)((__TIME__[1] * __TIME__[6]) + __TIME__[0]) << 48 |
				(uint64_t)((__TIME__[3] * __TIME__[6]) + __TIME__[4]) << 40 |
				(uint64_t)((__TIME__[4] * __TIME__[6]) + __TIME__[3]) << 32 |
				(uint64_t)((__TIME__[0] * __TIME__[7]) + __TIME__[1]) << 24 |
				(uint64_t)((__TIME__[1] * __TIME__[7]) + __TIME__[0]) << 16 |
				(uint64_t)((__TIME__[3] * __TIME__[7]) + __TIME__[4]) << 8 |
				(uint64_t)((__TIME__[4] * __TIME__[7]) + __TIME__[3]) << 0);

			auto ptr = (const uint8_t*)c + (i * 8);
			uint8_t ret[8] = {
				((i * 8) + 0) < size() ? ptr[0] : (uint8_t)0,
				((i * 8) + 1) < size() ? ptr[1] : (uint8_t)0,
				((i * 8) + 2) < size() ? ptr[2] : (uint8_t)0,
				((i * 8) + 3) < size() ? ptr[3] : (uint8_t)0,
				((i * 8) + 4) < size() ? ptr[4] : (uint8_t)0,
				((i * 8) + 5) < size() ? ptr[5] : (uint8_t)0,
				((i * 8) + 6) < size() ? ptr[6] : (uint8_t)0,
				((i * 8) + 7) < size() ? ptr[7] : (uint8_t)0
			}; return *(uint64_t*)&ret ^ key;
		}

	}

	template <size_t... indices>
	__forceinline constexpr xorstr(bool gg, const value_type(&str)[length], std::index_sequence<indices...>) :
		data{ crypt(gg, str, indices)... } { }

public:
	__forceinline constexpr xorstr(const value_type(&str)[length], bool local = true)
		: xorstr(local, str, std::make_index_sequence<count>()) {}

	__forceinline constexpr size_t size() {
		return sizeof(value_type[length]);
	}

	__forceinline const auto get() {
		return (const value_type*)data;
	}

	__forceinline const auto crypt_get()
	{
		constexpr volatile uint64_t key = (
			(uint64_t)((__TIME__[0] * __TIME__[6]) + __TIME__[1]) << 56 |
			(uint64_t)((__TIME__[1] * __TIME__[6]) + __TIME__[0]) << 48 |
			(uint64_t)((__TIME__[3] * __TIME__[6]) + __TIME__[4]) << 40 |
			(uint64_t)((__TIME__[4] * __TIME__[6]) + __TIME__[3]) << 32 |
			(uint64_t)((__TIME__[0] * __TIME__[7]) + __TIME__[1]) << 24 |
			(uint64_t)((__TIME__[1] * __TIME__[7]) + __TIME__[0]) << 16 |
			(uint64_t)((__TIME__[3] * __TIME__[7]) + __TIME__[4]) << 8 |
			(uint64_t)((__TIME__[4] * __TIME__[7]) + __TIME__[3]) << 0);

		volatile const size_t cnt = &data[count] - &data[0];
		for (size_t i = 0; i < cnt; data[i] ^= key, i++);
		return (const value_type*)data;
	}
};
#define E(s) xorstr(s).crypt_get()