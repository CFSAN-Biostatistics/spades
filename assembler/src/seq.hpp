/*
 * seq.hpp
 *
 *  Created on: 21.02.2011
 *      Author: vyahhi
 */

#ifndef SEQ_HPP_
/*
 * Sequence class with compile-time size
 *
 *  Created on: 20.02.2011
 *      Author: vyahhi
 */

#define SEQ_HPP_

#include <string>
#include <iostream> // for debug
#include <cassert>
#include <array>
#include <algorithm>
#include "nucl.hpp"
#include "log.hpp"

template <size_t size_, typename T = char> // max number of nucleotides, type for storage
class Seq {
private:
	// compile-time static constants.
	const static size_t Tbits = sizeof(T) << 3; // ex. 8: 2^8 = 256 or 16
	const static size_t Tnucl = sizeof(T) << 2; // ex. 4: 8/2 = 4 or 16/2 = 8
	const static size_t Tnucl_bits = log_<sizeof(T),2>::value + 2; // ex. 2: 2^2 = 4 or 3: 2^3 = 8
	const static size_t data_size_ = (size_ + Tnucl - 1) >> Tnucl_bits;
	std::array<T,data_size_> data_; // 0 bits overhead

	void init(const char* s) {
		T data = 0;
		size_t cnt = 0;
		int cur = 0;
		for (size_t pos = 0; pos < size_ && *s != 0; ++pos, ++s) { // unsafe!
			assert(is_nucl(*s));
			data |= (denucl(*s) << cnt);
			cnt += 2;
			if (cnt == Tbits) {
				this->data_[cur++] = data;
				cnt = 0;
				data = 0;
			}
		}
		if (cnt != 0) {
			this->data_[cur++] = data;
		}
	}

	Seq(std::array<T,data_size_> data): data_(data) {};

public:
	Seq() {
		std::fill(data_.begin(), data_.end(), 0);
	};

	Seq(const char* s) {
		init(s);
	}

	/*Seq(const Seq<size_,T> &seq): data_(seq.data_) {
		//does memcpy faster?
	}*/

	template <typename S> Seq(const S& s, size_t offset = 0) {
		char a[size_];
		for (size_t i = 0; i < size_; ++i) {
			a[i] = s[offset + i];
		}
		init(a);
	}

	template <size_t _bigger_size, T>
	Seq(const Seq<_bigger_size, T>& seq) {
		assert(_bigger_size > size_);
		std::copy(data_, seq.data_, size_);
		//memcpy(data_, seq._bytes, data_size_); faster?
	}

	char operator[] (const size_t index) const { // 0123
		assert(index >= 0);
		assert(index < size_);
		int ind = index >> Tnucl_bits;
		return (data_[ind] >> ((index % Tnucl)*2)) & 3;
	}

	/*
	 * reverse complement from the Seq
	 */
	Seq<size_,T> operator!() const { // TODO: optimize
		std::string s = this->str();
		std::reverse(s.begin(), s.end());
		std::transform(s.begin(), s.end(), s.begin(), denucl);
		std::transform(s.begin(), s.end(), s.begin(), complement);
		std::transform(s.begin(), s.end(), s.begin(), nucl);
		return Seq<size_,T>(s.c_str());
	}

	/**
	 * add one nucl to the right, shifting seq to the left
	 */
	Seq<size_,T> operator<<(char c) const {
		assert(is_nucl(c));
		Seq<size_, T> res(data_);
		if (data_size_ != 0) { // unless empty sequence
			T rm = res.data_[data_size_ - 1] & 3;
			res.data_[data_size_ - 1] >>= 2;
			T lastnuclshift_ = ((size_ + Tnucl - 1) % Tnucl) << 1;
			res.data_[data_size_ - 1] |= (denucl(c) << lastnuclshift_);
			if (data_size_ >= 2) { // if we have at least 2 elements in data
				size_t i = data_size_ - 1;
				do {
					--i;
					T new_rm = res.data_[i] & 3;
					res.data_[i] >>= 2;
					res.data_[i] &= (1 << (Tbits - 2)) - 1; // because if we shift negative, it fill with 1s :(
					res.data_[i] |= rm << (Tbits - 2);
					rm = new_rm;
				} while (i != 0);
			}
		}
		return res;
	}

	/**
	 * add one nucl to the left, shifting seq to the right
	 */
	Seq<size_> operator>>(char c) {	// TODO: optimize, better name
		assert(is_nucl(c));
		std::string s = c + this->str().substr(0, size_ - 1);
		return Seq<size_>(s.c_str());
	}

	bool operator==(Seq<size_, T> s) const {	// TODO: optimize
		return str() == s.str();
	}

	// string representation of Seq - only for debug and output purposes
	std::string str() const {
		std::string res(size_, '-');
		for (size_t i = 0; i < size_; ++i) {
			res[i] = nucl(this->operator[](i));
		}
		return res;
	}

	static int size() {
		return size_;
	}

	struct hash {
		 size_t operator() (const Seq<size_> &seq) const {
			size_t h = 0;
			for (size_t i = 0; i < data_size_; ++i) {
				h += seq.data_[i];
			}
			return h;
		}
	};

	struct equal_to {
		bool operator() (const Seq<size_> &l, const Seq<size_> &r) const {
			return l.data_ == r.data_;
			//return 0 == memcmp(l._bytes.data(), r._bytes.data(), _byteslen);
		}
	};

	struct less {
		int operator() (const Seq<size_> &l, const Seq<size_> &r) const {
			return l.data_ < r.data_;
			//return 0 > memcmp(l._bytes.data(), r._bytes.data(), _byteslen);
		}
	};

	template <int size2, typename T2 = char>
	Seq<size2,T2> head() { // TODO: optimize (Kolya)
		std::string s = str();
		return Seq<size2,T2>(s.substr(0, size2).c_str());
	}

	template <int size2, typename T2 = char>
	Seq<size2,T2> tail() const { // TODO: optimize (Kolya)
		std::string s = str();
		return Seq<size2,T2>(s.substr(size_ - size2, size2).c_str());
	}

};

// *****************************************
// LEGACY CODE

/*template <typename T2>
Seq(const T2& t, size_t offset = 0) {
	char a[size_];
	for (size_t i = 0; i < size_; ++i) {
		a[i] = t[offset + i];
	}
	init(a);
}*/

//	// add nucleotide to the right
//	Seq<_size> shift_right(char c) const { // char should be 0123
//		assert(c <= 3);
//		Seq<_size> res = *this; // copy constructor
//		c <<= (((4-(_size%4))%4)*2); // omg >.<
//		for (int i = _byteslen - 1; i >= 0; --i) { // don't make it size_t :)
//			char rm = (res._bytes[i] >> 6) & 3;
//			res._bytes[i] <<= 2;
//			//res._bytes[i] &= 252;
//			res._bytes[i] |= c;
//			c = rm;
//		}
//		return res;
//	}
//
//	// add nucleotide to the left
//	Seq<_size> shift_left(char c) const { // char should be 0123
//		Seq<_size> res = *this; // copy constructor
//		// TODO: clear last nucleotide
//		for (size_t i = 0; i < _byteslen; ++i) {
//			char rm = res._bytes[i] & 3;
//			res._bytes[i] >>= 2;
//			//res._bytes[i] &= 63;
//			res._bytes[i] |= (c << 6);
//			c = rm;
//		}
//		return res;
//	}

#endif /* SEQ_HPP_ */
