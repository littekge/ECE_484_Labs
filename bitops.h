#ifndef BITOPS
#define BITOPS

namespace BOP {
	
	//set1 methods
	inline void set1(volatile uint8_t& reg, uint8_t bitNum) {
		reg |= (1 << bitNum);
	}
	inline void set1(volatile uint8_t& reg, uint8_t bitNums[]) {
		for (uint8_t i = 1; i < sizeof(bitNums); i++) {
			reg |= (1 << bitNums[i]);
		}
	}
	
	//set0 methods
	inline void set0(volatile uint8_t& reg, uint8_t bitNum) {
		reg &= ~(1 << bitNum);
	}
	inline void set0(volatile uint8_t& reg, uint8_t bitNums[]) {
		for (uint8_t i = 1; i < sizeof(bitNums); i++) {
			reg &=~(1 << bitNums[i]);
		}
	}
	
	//check if a bit is 1
	inline bool check1(const uint8_t reg, const uint8_t bitNum) {
		return reg & (1 << bitNum);
	}
	
	//check if a bit is 0
	inline bool check0(const uint8_t reg, const uint8_t bitNum) {
		return !(reg & (1 << bitNum));
	}
	
	//toggle a bit
	inline void toggle(volatile uint8_t& reg, uint8_t bitNum) {
		reg ^= (1 << bitNum);
	}
	inline void toggle(volatile uint8_t& reg, uint8_t bitNums[]) {
		for (uint8_t i = 1; i < sizeof(bitNums); i++) {
			reg ^= (1 << bitNums[i]);
		}
	}
}


#endif