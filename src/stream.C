/*
Copyright (c) 1993 Dennis Heimbigner
All rights reserved.

This software was developed by Dennis Heimbigner
as part of the Arcadia project at
the University of Colorado, Boulder.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all such forms and that any documentation,
and other materials related to such
distribution and use acknowledge that the software was developed
by Dennis Heimbigner.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>
#include <stdarg.h>
#include "tps.H"
#include "mem.H"

EXTERNC int fsync(int);

#if !defined(__osf__) && !defined(solaris2)
#  ifndef __GNUC__
EXTERNC int open(char*,int...);
#  endif
#endif

/**************************************************/
/* Utilities */

Tps_Stream_Mode
Tps_mode_of(const char* modestring, const long modelen)
{
    register Tps_Stream_Mode md = 0;
    if(modelen != 1) return md;
    switch (*modestring) {
	    case 'r': md = Tps_stream_r; break;
	    case 'w': md = Tps_stream_w; break;
	    case 'a': md = Tps_stream_a; break;
	default: break;
    }
    return md;
}

char*
Tps_modestring_of(const Tps_Stream_Mode md)
{
    switch (md) {
	case Tps_stream_r: return "r";
	case Tps_stream_w: return "w";
	case Tps_stream_a: return "a";
	default: return "?";
    }
/*NOTREACHED*/
}

/**************************************************/
/**************************************************/
/* File type operations */
 
Tps_Status
Tps_Stream_File::init(const char* name, const long namelen,
		      const Tps_Stream_Mode md)
{
    _attached = FALSE;
    _modeflags = md;
    _fd = -1;
    _pushed = -1;
    _f = (void*)0;
    if(_name) Tps_free(_name);
    _name = (char*)0;
    if(name && namelen > 0) {
	_name = Tps_malloc(namelen+1);
	if(!_name) return TPSSTAT_VMERROR;
	MEMCPY(_name,name,namelen);
	/* force null termination*/
	_name[namelen] = 0;
    }
    return TPSSTAT_OK;
}

Tps_Stream_File::~Tps_Stream_File()
{
    (void)close();
    Tps_free(_name);
}

Tps_Status
Tps_Stream_File::open(const char* fname, const long flen, const Tps_Stream_Mode md)
{
    if(_isopen) return TPSSTAT_IOERROR;
    register Tps_Status ok = init(fname,flen,md);
    if(ok != TPSSTAT_OK) return ok;
    switch (_modeflags) {
	case Tps_stream_r: _fd = ::open(_name,O_RDONLY); break;
	case Tps_stream_w: _fd = ::open(_name,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU); break;
	default: break;
    }
    _good = (_isopen = (_fd >= 0));
    return _isopen?TPSSTAT_OK:TPSSTAT_INVALIDSTREAMACCESS;
}

Tps_Status
Tps_Stream_File::attach(const int f, const char* fname, const Tps_Stream_Mode md)
{
    if(_isopen || f < 0) return TPSSTAT_INVALIDSTREAMACCESS;
    register Tps_Status ok = init(fname,strlen(fname),md);
    if(ok != TPSSTAT_OK) return ok;
    _fd = f;
    _attached = TRUE;
    _good = (_isopen = TRUE);
    return _isopen?TPSSTAT_OK:TPSSTAT_INVALIDSTREAMACCESS;
}

Tps_Status
Tps_Stream_File::close()
{
    if(!_isopen) return TPSSTAT_INVALIDSTREAMACCESS;
    (void)::close(_fd);
    _isopen = FALSE;
    _good = FALSE;
    return TPSSTAT_OK;
}

/**************************************************/

long
Tps_Stream_File::read()
{
    char s[1];
    long c;
    if(!_good) return TPSSTAT_IOERROR;
    if(!r()) return TPSSTAT_INVALIDSTREAMACCESS;
    if(_pushed < 0) {
	switch (::read(_fd,s,1)) {
	    case -1: _good = FALSE; /*FALLTHRU*/
	    case 0: c = EOF; break;
	    case 1: c = s[0]; break;
	}
    } else {
	c = (char)_pushed;
	_pushed = -1;
    }
if(!c) abort();
    return c;
}

Tps_Status
Tps_Stream_File::write(long c)
{
    char x[1]; 
    x[0] = (char)(c & 0xff);
    if(!_good) {
	return TPSSTAT_IOERROR;
    }
    if(!w()) {
	return TPSSTAT_INVALIDSTREAMACCESS;
    }
    if(::write(_fd,x,1) != 1) {
	_good = FALSE;
    }
    return good();
}

Tps_Status
Tps_Stream_File::write(const char* s, long slen)
{
    if(!_good) {
	return TPSSTAT_IOERROR;
    }
    if((!s && slen > 0) || !w()) {
	return TPSSTAT_INVALIDSTREAMACCESS;
    }
    if(!s) slen = 0;
    else if(slen < 0) slen = strlen(s);
    if(slen > 0) {
	if(::write(_fd,s,(int)slen) != slen) {
	    _good = FALSE;
	}
    }
    return good();
}

Tps_Status
Tps_Stream_File::pushback(long c)
{
    if(!_good) return TPSSTAT_IOERROR;
    if(!r() || _pushed >= 0) return TPSSTAT_INVALIDSTREAMACCESS;
    if(c >= 0) _pushed = c;
    return good();
}

Tps_Status
Tps_Stream_File::rewind()
{
    if(!_good) return TPSSTAT_IOERROR;
    _good = (::lseek(_fd,0,SEEK_SET) >= 0);
    return good();
}

Tps_Status
Tps_Stream_File::flush()
{
    if(!_good) return TPSSTAT_IOERROR;
    _good = (::fsync(_fd) >= 0);
    return good();
}

long
Tps_Stream_File::bytesavailable()
{
    register long len;
    register long current;
    if(!_good) return TPSSTAT_IOERROR;
    if(w()) return -1;
    if((current = ::lseek(_fd,0,SEEK_CUR)) < 0) return -1; // save position
    len = ::lseek(_fd,0,SEEK_END); // move to end
    (void)::lseek(_fd,current,SEEK_SET); // restore to original position
    if(len < 0) return -1;
    return len;
}

/*VARARGS0*/
Tps_Status
Tps_Stream_File::printf(const char* fmt...)
{
    va_list args;
    register FILE* f = (FILE*)_f;

    if(!_good || !w()) return TPSSTAT_IOERROR;
    if(!f) {
	f = fdopen(_fd,"w");
	_f = (void*)f;
    }
    /* print out remainder of message */
    va_start(args,fmt);
    (void) vfprintf(f, fmt, args);
    va_end(args);
    fflush(f); // keep in synch with the other write operations.
    return TPSSTAT_OK;
}

/**************************************************/
/**************************************************/

/* String stream */

Tps_Stream_String::Tps_Stream_String()
		   : Tps_Stream(Tps_stream_string)
{
    _eos = (_finger = (_contents = (char*)0));
    _alloc = 0;
}

Tps_Stream_String::~Tps_Stream_String()
{
    (void)close();
}

Tps_Status
Tps_Stream_String::open() // for writing
{
    if(_isopen) {  
	if(r()) return TPSSTAT_INVALIDSTREAMACCESS;
	return rewind(); // just start overwriting buffer
    }
    _modeflags = Tps_stream_w | Tps_stream_r;
    _good = (_isopen = TRUE);
    return good();
}

Tps_Status
Tps_Stream_String::open(const char* s, long slen) // for reading only
{
    if(slen < 0) {
	_good = FALSE;
    } else {
	if(!s) {s = ""; slen = 0;}
	if(_isopen && w()) {
	    Tps_free(_contents);
	    _alloc = 0;
	}
	_modeflags = Tps_stream_r;
	_good = (_isopen = TRUE);
	_finger = (_contents = s);
	_alloc = slen;
	_eos = _contents + _alloc;
    }
    return good();
}

Tps_Status
Tps_Stream_String::close()
{
    if(!_isopen) return TPSSTAT_OK;
    if(w()) Tps_free(_contents);
    _finger = (_eos = (_contents = (char*)0));
    _alloc = 0;
    _good = (_isopen = FALSE);
    return TPSSTAT_OK;
}

/**************************************************/

long
Tps_Stream_String::read()
{
    if(!_good) return TPSSTAT_IOERROR;
    if(!r()) return TPSSTAT_INVALIDSTREAMACCESS;
    if(_finger == _eos) return EOF;
    return *_finger++;
}

Tps_Status
Tps_Stream_String::write(long c)
{
    if(!_good) {
	return TPSSTAT_IOERROR;
    }
    if(!w()) {
	return TPSSTAT_INVALIDSTREAMACCESS;
    }
    if(!_contents || _finger == _eos) {
	if(!guarantee(16)) {
	    _good = FALSE;
	    return TPSSTAT_VMERROR;
	}
    }
    *_finger++ = (char)c;
    return good();
}

Tps_Status
Tps_Stream_String::write(const char* s, long slen)
{
    if(!_good) {
	return TPSSTAT_IOERROR;
    }
    if(slen < 0 || !w()) {
	return TPSSTAT_INVALIDSTREAMACCESS;
    }
    if(slen > 0) {
	if(!_contents || _finger + slen >= _eos) {
	    if(!guarantee(slen)) {
		_good = FALSE;
		return TPSSTAT_VMERROR;
	    }
	}
	MEMCPY(_finger,s,slen);
	_finger += slen;
    }
    return good();
}

Tps_Status
Tps_Stream_String::pushback(long)
{
    if(!_good) return TPSSTAT_IOERROR;
    if(!r()) return TPSSTAT_INVALIDSTREAMACCESS;
    if(_finger == _contents) {
	_good = FALSE;
	return TPSSTAT_IOERROR;
    }
    _finger--;
    return good();
}

Tps_Status
Tps_Stream_String::rewind()
{
    if(!_good) return TPSSTAT_IOERROR;
    _finger = _contents;
    return good();
}

Tps_Status
Tps_Stream_String::flush()
{
    return good();
}

long
Tps_Stream_String::bytesavailable()
{
    if(!_isopen) return TPSSTAT_OK;
    if(w() || !_contents) return -1;
    return _eos - _finger;
}

/*VARARGS0*/
Tps_Status
Tps_Stream_String::printf(const char* fmt...)
{
    va_list args;
    register long len;

    if(!_good || !w()) return TPSSTAT_IOERROR;
    len = strlen(fmt);
    if(!guarantee(len*2)) return TPSSTAT_VMERROR;
    /* print out remainder of message */
    va_start(args,fmt); // putting in a constant 1 fails under ATT C++
    (void) vsprintf(_finger, fmt, args);
    va_end(args);
    /* advance _finger to eos */
    fmt = (char*)MEMCHR(_finger,'\0',(_eos - _finger));
    if(!fmt) {
	/* uh-oh, overwrote available memory */
	_good = FALSE;
	return TPSSTAT_SYSTEMERROR;
    }
    _finger = fmt;
    return TPSSTAT_OK;
}

/**************************************************/

boolean
Tps_Stream_String::guarantee(long need)
{
    register long avail;
    register long used;
    register long newalloc;
    register char* newbuf;

    if(!w()) return FALSE;
    if(_contents) {
	avail = _eos - _finger;
	used = _alloc - avail;
	if(avail >= need) return TRUE;
	newalloc = _alloc + need; // slight overkill
	newbuf = Tps_malloc(newalloc);
	if(!newbuf) return FALSE;
	MEMCPY(newbuf,_contents,_alloc);
	Tps_free(_contents);
	_contents = newbuf;
	_alloc = newalloc;
	_finger = _contents + used;
	_eos = _contents + _alloc;
    } else {
	_alloc = need;
	_eos = (_finger = (_contents = Tps_malloc(_alloc)));
	if(!_contents) return FALSE;
	_eos += _alloc;
    }
    return TRUE;
}

/**************************************************/
/* Looks like virtual functions dont always work with g++ */
#ifdef __sgi
Tps_Status Tps_Stream::open() {return TPSSTAT_SYSTEMERROR;}
Tps_Status Tps_Stream::close() {return TPSSTAT_SYSTEMERROR;}
long Tps_Stream::read() {return -1;}
Tps_Status Tps_Stream::write(long) {return TPSSTAT_SYSTEMERROR;}
Tps_Status Tps_Stream::write(const char*, long) {return TPSSTAT_SYSTEMERROR;}
Tps_Status Tps_Stream::pushback(long) {return TPSSTAT_SYSTEMERROR;}
Tps_Status Tps_Stream::flush() {return TPSSTAT_SYSTEMERROR;}
Tps_Status Tps_Stream::rewind() {return TPSSTAT_SYSTEMERROR;}
long Tps_Stream::bytesavailable() {return -1;}
Tps_Status Tps_Stream::printf(const char* ...) {return TPSSTAT_SYSTEMERROR;}
Tps_Status Tps_Stream::ends() {return TPSSTAT_SYSTEMERROR;}
#endif /*__sgi*/
