/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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

/// main namespace
namespace CLRX
{

/*
 * stream utilities
 */

class MemoryStreamBuf: public std::streambuf
{
protected:
    std::ios_base::openmode openMode;
    
    MemoryStreamBuf(std::ios_base::openmode openMode);
    
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
             std::ios_base::openmode which);
    
    pos_type seekpos(pos_type pos, std::ios_base::openmode which);
    std::streamsize showmanyc();
    int_type pbackfail(int_type ch);
    
    void safePBump(ssize_t offset);
public:
    ~MemoryStreamBuf() = default;
};

/// array stream buffer that holds external static array for memory saving
class ArrayStreamBuf: public MemoryStreamBuf
{
public:
    ArrayStreamBuf(size_t size, char* buffer, std::ios_base::openmode openMode);
    ~ArrayStreamBuf() = default;
    
    /// get a held array size
    size_t getArraySize() const
    { return egptr()-eback(); }
    /// get a held array content
    char* getArray() const
    { return eback(); }
protected:
    std::streambuf* setbuf(char_type* buffer, std::streamsize size);
};

/// string stream buffer that holds external string for memory saving
class StringStreamBuf: public MemoryStreamBuf
{
private:
    std::string& string;
public:
    StringStreamBuf(std::string& string, std::ios_base::openmode openMode);
    ~StringStreamBuf() = default;
    
    /// get a held string
    std::string& getString() const
    { return string; }
    /// get a held string
    std::string& getString()
    { return string; }
protected:
    int_type overflow(int_type ch);
    std::streambuf* setbuf(char_type* buffer, std::streamsize size);
};

/// vector char stream buffer external char-vector for memory saving
class VectorStreamBuf: public MemoryStreamBuf
{
private:
    std::vector<char>& vector;
public:
    VectorStreamBuf(std::vector<char>& vector, std::ios_base::openmode openMode);
    ~VectorStreamBuf() = default;
    
    /// get a held char vector
    std::vector<char>& getVector() const
    { return vector; }
    /// get a held char vector
    std::vector<char>& getVector()
    { return vector; }
protected:
    int_type overflow(int_type ch);
    std::streambuf* setbuf(char_type* buffer, std::streamsize size);
};

/// specialized input stream that holds external array for memory saving
class ArrayIStream: public std::istream
{
private:
    ArrayStreamBuf buffer;
public:
    ArrayIStream(size_t size, const char* array);
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
    ArrayIOStream(size_t size, char* array);
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
    ArrayOStream(size_t size, char* array);
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
    StringIStream(const std::string& string);
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
    StringIOStream(std::string& string);
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
    StringOStream(std::string& string);
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
    VectorIStream(const std::vector<char>& string);
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
    VectorIOStream(std::vector<char>& string);
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
    VectorOStream(std::vector<char>& string);
    ~VectorOStream() = default;
    
    /// get a held char vector
    std::vector<char>& getVector() const
    { return buffer.getVector(); }
};

/*
 * adaptor
 */

struct BinaryOStream
{
private:
    std::ostream& os;
public:
    BinaryOStream(std::ostream& _os) : os(_os)
    { }
    
    template<typename T>
    void writeObject(const T& t)
    { os.write(reinterpret_cast<const char*>(&t), sizeof(T)); }
    
    template<typename T>
    void writeArray(size_t size, const T* t)
    { os.write(reinterpret_cast<const char*>(t), sizeof(T)*size); }
    
    const std::ostream& getOStream() const
    { return os; }
    std::ostream& getOStream()
    { return os; }
};

/// fast and direct output buffer
class FastOutputBuffer
{
private:
    std::ostream& os;
    cxuint endPos;
    cxuint bufSize;
    std::unique_ptr<char[]> buffer;
public:
    FastOutputBuffer(cxuint inBufSize, std::ostream& output) : os(output), endPos(0),
            bufSize(inBufSize), buffer(new char[inBufSize])
    { }
    
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
    
    /// go forward
    void forward(cxuint toWrite)
    { endPos += toWrite; }
    
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
    }
    
    void writeString(const char* string)
    { write(::strlen(string), string); }
    
    template<typename T>
    void writeObject(const T& t)
    { write(sizeof(T), reinterpret_cast<const char*>(&t)); }
    
    template<typename T>
    void writeArray(size_t size, const T* t)
    { write(sizeof(T)*size, reinterpret_cast<const char*>(t)); }
    
    void put(char c)
    {
        if (endPos == bufSize)
            flush();
        buffer[endPos++] = c;
    }
};

}

#endif
