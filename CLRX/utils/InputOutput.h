/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*! \file InputOutput.h
 * \brief input output utilities
 */

#ifndef __CLRX_INPUTOUTPUT_H__
#define __CLRX_INPUTOUTPUT_H__

#include <CLRX/Config.h>
#include <vector>
#include <cstring>
#include <istream>
#include <ostream>
#include <memory>
#include <streambuf>
#include <CLRX/utils/Utilities.h>

/// main namespace
namespace CLRX
{

/*
 * stream utilities
 */

/// memory stream buffer
class MemoryStreamBuf: public std::streambuf
{
protected:
    std::ios_base::openmode openMode;   ///< open mode
    
    /// constructor
    MemoryStreamBuf(std::ios_base::openmode openMode);
    
    /// seekoff implementation
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
             std::ios_base::openmode which);
    
    /// seekpos implementation
    pos_type seekpos(pos_type pos, std::ios_base::openmode which);
    /// showmanyc implementation
    std::streamsize showmanyc();
    /// pbackfail implementation
    int_type pbackfail(int_type ch);
    
    /// safe pbump version
    void safePBump(ssize_t offset);
public:
    /// destructor
    ~MemoryStreamBuf() = default;
};

/// array stream buffer that holds external static array for memory saving
class ArrayStreamBuf: public MemoryStreamBuf
{
public:
    /// constructor
    ArrayStreamBuf(size_t size, char* buffer, std::ios_base::openmode openMode);
    /// destructor
    ~ArrayStreamBuf() = default;
    
    /// get a held array size
    size_t getArraySize() const
    { return egptr()-eback(); }
    /// get a held array content
    char* getArray() const
    { return eback(); }
protected:
    /// setbuf implementation
    std::streambuf* setbuf(char_type* buffer, std::streamsize size);
};

/// string stream buffer that holds external string for memory saving
class StringStreamBuf: public MemoryStreamBuf
{
private:
    std::string& string;
public:
    /// constructor
    StringStreamBuf(std::string& string, std::ios_base::openmode openMode);
    /// destructor
    ~StringStreamBuf() = default;
    
    /// get a held string
    std::string& getString() const
    { return string; }
    /// get a held string
    std::string& getString()
    { return string; }
protected:
    /// overflow implementation
    int_type overflow(int_type ch);
    /// setbuf implementation
    std::streambuf* setbuf(char_type* buffer, std::streamsize size);
};

/// vector char stream buffer external char-vector for memory saving
class VectorStreamBuf: public MemoryStreamBuf
{
private:
    std::vector<char>& vector;
public:
    /// constructor
    VectorStreamBuf(std::vector<char>& vector, std::ios_base::openmode openMode);
    /// destructor
    ~VectorStreamBuf() = default;
    
    /// get a held char vector
    std::vector<char>& getVector() const
    { return vector; }
    /// get a held char vector
    std::vector<char>& getVector()
    { return vector; }
protected:
    /// overflow implementation
    int_type overflow(int_type ch);
    /// setbuf implementation
    std::streambuf* setbuf(char_type* buffer, std::streamsize size);
};

/// specialized input stream that holds external array for memory saving
class ArrayIStream: public std::istream
{
private:
    ArrayStreamBuf buffer;
public:
    /// constructor
    ArrayIStream(size_t size, const char* array);
    /// destructor
    ~ArrayIStream() = default;
    
    /// get a held array size
    size_t getArraySize() const
    { return buffer.getArraySize(); }
    /// get a held array content
    const char* getArray() const
    { return buffer.getArray(); }
};

/// specialized input/output stream that holds external array for memory saving
class ArrayIOStream: public std::iostream
{
private:
    ArrayStreamBuf buffer;
public:
    /// constructor
    ArrayIOStream(size_t size, char* array);
    /// destructor
    ~ArrayIOStream() = default;
    
    /// get a held array size
    size_t getArraySize() const
    { return buffer.getArraySize(); }
    /// get a held array content
    char* getArray() const
    { return buffer.getArray(); }
};

/// specialized output stream that holds external array for memory saving
class ArrayOStream: public std::ostream
{
private:
    ArrayStreamBuf buffer;
public:
    /// constructor
    ArrayOStream(size_t size, char* array);
    /// destructor
    ~ArrayOStream() = default;
    
    /// get a held array size
    size_t getArraySize() const
    { return buffer.getArraySize(); }
    /// get a held array content
    char* getArray() const
    { return buffer.getArray(); }
};

/// specialized input stream that holds external string for memory saving
class StringIStream: public std::istream
{
private:
    StringStreamBuf buffer;
public:
    /// constructor
    StringIStream(const std::string& string);
    /// destructor
    ~StringIStream() = default;
    
    /// get a held string
    const std::string& getString() const
    { return buffer.getString(); }
};

/// specialized input/output stream that holds external string for memory saving
class StringIOStream: public std::iostream
{
private:
    StringStreamBuf buffer;
public:
    /// constructor
    StringIOStream(std::string& string);
    /// destructor
    ~StringIOStream() = default;
    
    /// get a held string
    std::string& getString() const
    { return buffer.getString(); }
};

/// specialized output stream that holds external string for memory saving
class StringOStream: public std::ostream
{
private:
    StringStreamBuf buffer;
public:
    /// constructor
    StringOStream(std::string& string);
    /// destructor
    ~StringOStream() = default;
    
    /// get a held string
    std::string& getString() const
    { return buffer.getString(); }
};

/// specialized input stream that holds external char-vector for memory saving
class VectorIStream: public std::istream
{
private:
    VectorStreamBuf buffer;
public:
    /// constructor
    VectorIStream(const std::vector<char>& string);
    /// destructor
    ~VectorIStream() = default;
    
    /// get a held char vector
    const std::vector<char>& getVector() const
    { return buffer.getVector(); }
};

/// specialized input/output stream that holds external char-vector for memory saving
class VectorIOStream: public std::iostream
{
private:
    VectorStreamBuf buffer;
public:
    /// constructor
    VectorIOStream(std::vector<char>& string);
    /// destructor
    ~VectorIOStream() = default;
    
    /// get a held char vector
    std::vector<char>& getVector() const
    { return buffer.getVector(); }
};

/// specialized output stream that holds external char-vector for memory saving
class VectorOStream: public std::ostream
{
private:
    VectorStreamBuf buffer;
public:
    /// constructor
    VectorOStream(std::vector<char>& string);
    /// destructor
    ~VectorOStream() = default;
    
    /// get a held char vector
    std::vector<char>& getVector() const
    { return buffer.getVector(); }
};

/*
 * adaptor
 */

/// fast input buffer adapter
class FastInputBuffer: public NonCopyableAndNonMovable
{
private:
    std::istream& is;
    cxuint pos, endPos;
    cxuint bufSize;
    std::unique_ptr<char[]> buffer;
public:
    /// constructor
    /**
     * \param _bufSize buffer size
     * \param input input stream
     */
    FastInputBuffer(cxuint _bufSize, std::istream& input) : is(input), pos(0), endPos(0),
            bufSize(_bufSize), buffer(new char[_bufSize])
    { }
    
    /// get input stream
    const std::istream& getIStream() const
    { return is; }
    /// get input stream
    std::istream& getIStream()
    { return is; }
    
    /// get character or returns eof()
    int get()
    {
        if (pos == endPos)
        {
            pos = 0;
            is.read(buffer.get(), bufSize);
            endPos = is.gcount();
            if (endPos == 0 || is.bad())
                return std::streambuf::traits_type::eof();
        }
        return (cxuchar)buffer.get()[pos++];
    }
    
    /// read data from buffer and returns number of read bytes
    size_t read(char* buf, cxuint n)
    {
        size_t toRead = std::min(n, endPos-pos);
        ::memcpy(buf, buffer.get()+pos, toRead);
        if (toRead < n)
        {
            is.read(buf+toRead, n-toRead);
            toRead += is.gcount();
            pos = endPos = 0;
        }
        return toRead;
    }
};

/// fast and direct output buffer
class FastOutputBuffer: public NonCopyableAndNonMovable
{
private:
    std::ostream& os;
    cxuint endPos;
    cxuint bufSize;
    std::unique_ptr<char[]> buffer;
    uint64_t written;
public:
    /// constructor with inBufSize and output
    /**
     * \param _bufSize max buffer size
     * \param output output stream
     */
    FastOutputBuffer(cxuint _bufSize, std::ostream& output) : os(output), endPos(0),
            bufSize(_bufSize), buffer(new char[_bufSize]), written(0)
    { }
    /// destructor
    ~FastOutputBuffer()
    { 
        flush();
        os.flush();
    }
    
    /// get written bytes number
    uint64_t getWritten() const
    { return written; }
    
    /// write output buffer
    void flush()
    {
        os.write(buffer.get(), endPos);
        endPos = 0;
    }
    
    /// reserve and write out buffer if too few free bytes in buffer
    char* reserve(cxuint toReserve)
    {
        if (toReserve > bufSize-endPos)
            flush();
        return buffer.get() + endPos;
    }
    
    /// finish reservation and go forward
    void forward(cxuint toWrite)
    {
        endPos += toWrite;
        written += toWrite;
    }
    
    /// write sequence of characters
    void write(size_t length, const char* string)
    {
        if (length > bufSize-endPos)
        {
            flush();
            os.write(string, length);
        }
        else
        {
            ::memcpy(buffer.get()+endPos, string, length);
            endPos += length;
        }
        written += length;
    }
    
    /// write string
    void writeString(const char* string)
    { write(::strlen(string), string); }
    
    /// write object of type T
    template<typename T>
    void writeObject(const T& t)
    { write(sizeof(T), reinterpret_cast<const char*>(&t)); }
    
    /// write array of objects
    template<typename T>
    void writeArray(size_t size, const T* t)
    { write(sizeof(T)*size, reinterpret_cast<const char*>(t)); }
    
    /// put single character
    void put(char c)
    {
        if (endPos == bufSize)
            flush();
        buffer[endPos++] = c;
        written++;
    }
    
    /// fill (put num c character)
    void fill(size_t num, char c)
    {
        size_t count = num;
        while (count != 0)
        {
             size_t bufNum = std::min(size_t(bufSize-endPos), count);
             ::memset(buffer.get()+endPos, c, bufNum);
             count -= bufNum;
             endPos += bufNum;
             if (endPos == bufSize)
                 flush();
        }
        written += num;
    }
    
    /// get output stream
    const std::ostream& getOStream() const
    { return os; }
    /// get output stream
    std::ostream& getOStream()
    { return os; }
};

};

#endif
