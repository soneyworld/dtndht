/*
 * HashStream.cpp
 *
 *  Created on: 08.07.2010
 *      Author: Till Lorentzen
 */
#include "HashStream.h"
#ifdef DO_DEBUG_OUTPUT
#include <stdio>
#endif
HashStream::HashStream(const char * hashname) :
	ostream(this), out_buf_(new char[BUFF_SIZE]), hash_(
			new unsigned char[EVP_MAX_MD_SIZE]) {
	// Initialize get pointer.  This should be zero so that underflow is called upon first read.
	setp(out_buf_, out_buf_ + BUFF_SIZE - 1);
	OpenSSL_add_all_digests();
	md = EVP_get_digestbyname(hashname);
	if (!md) {
#ifdef DO_DEBUG_OUTPUT
		std::cerr << "Unknown message digest: " << hashname
				<< "\nChanging to default: md5" << std::endl();
#endif
		md = EVP_get_digestbyname("md5");
	}
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);
}

HashStream::~HashStream() {
	delete[] out_buf_;

	EVP_MD_CTX_cleanup(&mdctx);
}

const unsigned char* HashStream::getHash() const {
	return hash_;
}

unsigned int HashStream::getHashLength() const {
	return md->md_size;
}

void HashStream::reset() {
	EVP_MD_CTX_cleanup(&mdctx);

	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);
}

int HashStream::sync() {
	int ret = std::char_traits<char>::eq_int_type(this->overflow(
			std::char_traits<char>::eof()), std::char_traits<char>::eof()) ? -1
			: 0;
	unsigned int md_len;
	EVP_DigestFinal_ex(&mdctx, hash_, &md_len);
	return ret;
}

int HashStream::overflow(int c) {
	char *ibegin = out_buf_;
	char *iend = pptr();

	// mark the buffer as free
	setp(out_buf_, out_buf_ + BUFF_SIZE - 1);

	if (!std::char_traits<char>::eq_int_type(c, std::char_traits<char>::eof())) {
		*iend++ = std::char_traits<char>::to_char_type(c);
	}

	// if there is nothing to send, just return
	if ((iend - ibegin) == 0) {
		return std::char_traits<char>::not_eof(c);
	}

	// hashing
	EVP_DigestUpdate(&mdctx, (unsigned char*) out_buf_, (iend - ibegin));
	return std::char_traits<char>::not_eof(c);
}
