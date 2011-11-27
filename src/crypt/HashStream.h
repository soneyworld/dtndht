/*
 * HashStream.h
 *
 *  Created on: 08.07.2010
 *      Author: Till Lorentzen
 */

#ifndef HASHSTREAM_H_
#define HASHSTREAM_H_
#include "ibrcommon/Exceptions.h"
#include <streambuf>
#include <iostream>
#include <openssl/evp.h>
class HashStream : public std::basic_streambuf<char, std::char_traits<char> >, public std::ostream
	{
	public:
		static const size_t BUFF_SIZE = 2048;
		HashStream(const char * hashname);
		virtual ~HashStream();

		const unsigned char* getHash() const;
		unsigned int getHashLength() const;

		void reset();

	protected:
		virtual int sync();
		virtual int overflow(int = std::char_traits<char>::eof());

	private:
		// Output buffer
		char *out_buf_;
		const EVP_MD *md;
		EVP_MD_CTX mdctx;
		unsigned char * hash_;
		unsigned int hash_size_;
	};

#endif /* HASHSTREAM_H_ */
