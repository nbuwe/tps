/*
 * Copyright (c) 1993 Dennis Heimbigner
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef STREAM_HH
#define STREAM_HH 1

#include "container.H"

#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**************************************************/
/* Sadly, the C++ streams interface does not provide
   a sufficiently consistent interface (e.g is_open()
   only works on file streams) and not appear to
   export enough info (e.g. the mode in which a file
   was opened.
*/
/**************************************************/
/*
Enumerate the possible realizations of streams:
*/

typedef enum Tps_Stream_Type {
	Tps_stream_file=0,
	Tps_stream_string=1	/* internal string that expands on write() */
} Tps_Stream_Type;

/* following designed to be tested by masking */
typedef unsigned int Tps_Stream_Mode;

const Tps_Stream_Mode Tps_stream_r	= 1 << 0;
const Tps_Stream_Mode Tps_stream_w	= 1 << 1;
const Tps_Stream_Mode Tps_stream_a	= 1 << 2;

EXTERNC Tps_Stream_Mode Tps_mode_of(const char* modestring, const long modelen);
EXTERNC const char* Tps_modestring_of(const Tps_Stream_Mode);

class Tps_Stream : public Tps_Container {
    public:
	Tps_Stream(Tps_Stream_Type t) : Tps_Container(TPSTYPE_STREAM) {
	    _type = t;
	    _name = (char*)0;
	    _isopen = FALSE;
	    _good = FALSE;
	    _modeflags = 0;
	}
	virtual ~Tps_Stream();
    public:
	virtual Tps_Status open() = 0;
	virtual Tps_Status close() = 0;
    public:
	virtual long read() = 0;
	virtual Tps_Status write(long c) = 0;
	virtual Tps_Status write(const char*, long) = 0;
	virtual Tps_Status pushback(long c) = 0;
	virtual Tps_Status flush() = 0;
	virtual Tps_Status rewind() = 0;
	virtual long bytesavailable() = 0;
    public:
	virtual Tps_Status printf(const char* fmt...) = 0;
	virtual Tps_Status ends() = 0;
    public:
	Tps_Status write(const char* s) {return write(s,s?strlen(s):0);}
	Tps_Status endl() {(void)ends(); return flush();}
    public:
	const char* name() const {return _name;}
	boolean isopen() const {return _isopen;}
	Tps_Status good() const {return (_isopen && _good)?TPSSTAT_OK:TPSSTAT_IOERROR;}
	boolean operator!() const {return good() != TPSSTAT_OK;}
	Tps_Stream_Mode mode() const {return _modeflags;}
	const char* modestring() const{return Tps_modestring_of(_modeflags);}
	Tps_Stream_Type stream_type() const {return _type;}
	boolean r() const {return (_modeflags & Tps_stream_r)?1:0;}
	boolean w() const {return (_modeflags & (Tps_stream_w | Tps_stream_a))?1:0;}
	boolean a() const {return (_modeflags & Tps_stream_a)?1:0;}
    protected:
	Tps_Stream_Type	_type;
	char* 		_name;
	int		_isopen;
	int		_good;
	Tps_Stream_Mode	_modeflags;
};

class Tps_Stream_File : public Tps_Stream {
    public:
	Tps_Stream_File()
		: Tps_Stream(Tps_stream_file) {(void)open();}
	virtual ~Tps_Stream_File();
    public:
	virtual Tps_Status open()
		{return init((char*)0,0,0);}
	virtual Tps_Status open(const char* fname, const long flen,
				const Tps_Stream_Mode md=Tps_stream_r);
	virtual Tps_Status open(const char* fname,
				const Tps_Stream_Mode md=Tps_stream_r)
		{return open(fname,strlen(fname),md);}
	virtual Tps_Status attach(const int f, const char* fname,
				const Tps_Stream_Mode md=Tps_stream_r);
	virtual Tps_Status close();
    public:
	virtual long read();
	using Tps_Stream::write;
	virtual Tps_Status write(long c);
	virtual Tps_Status write(const char*,long);
	virtual Tps_Status pushback(long c);
	virtual Tps_Status flush();
	virtual Tps_Status rewind();
	virtual long bytesavailable();
    public:
	virtual Tps_Status printf(const char* fmt...);
	virtual Tps_Status ends() {return TPSSTAT_OK;} /* avoid nulls*/
    public:
	int fd() {return _fd;}
	long attached() {return _attached;}
    protected:
	Tps_Status init(const char*, const long, const Tps_Stream_Mode);
    protected:
	int _fd;
	long _attached;
	long _pushed; /* for pushedback */
	void*	_f; /* really FILE* _f attached to _fd */
};

class Tps_Stream_String : public Tps_Stream {
    public:
	Tps_Stream_String();
	virtual ~Tps_Stream_String();
    public:
	virtual Tps_Status open(); // for writing and reading
	virtual Tps_Status open(const char* buf, long buflen); // for reading only
	Tps_Status open(const char* buf) {return open(buf,strlen(buf));}
	virtual Tps_Status close();
    public:
	virtual long read();
	using Tps_Stream::write;
	virtual Tps_Status write(long c);
	virtual Tps_Status write(const char*,long);
	virtual Tps_Status pushback(long c);
	virtual Tps_Status flush();
	virtual Tps_Status rewind();
	virtual long bytesavailable();
    public:
	virtual Tps_Status printf(const char* fmt...);
	virtual Tps_Status ends() {return write((long)0);}
    public:
	char* contents() {return _contents;}
	long length() {return _finger - _contents;}
    public:
	boolean guarantee(long need);
    protected:
	char*	_contents;
	long	_alloc; /* current allocated size of _contents */
	char*	_finger; /* place to read-next or write-next */
	char*	_eos; /* usually _contents+_alloc */
};

#endif /*STREAM_HH*/
