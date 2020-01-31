#pragma once

#include <Arduino.h>
#include <FS.h>

/**
 * Saves binary data blocks circular buffered in a file.
 */
template <class TTime, class TData>
class ggCircularFileT {

public:

  ggCircularFileT(const String& aFileName,
                  uint32_t aNumberOfDataBlocks,
                  FS* aFileSystem)
  : mFileSystem(aFileSystem),
    mFileName(aFileName),
    mNumberOfDataBlocks(aNumberOfDataBlocks),
    mIndex(aNumberOfDataBlocks) {
  }

  const String& GetFileName() const {
    return mFileName;
  }

  uint32_t GetNumberOfDataBlocks() const {
    return mNumberOfDataBlocks;
  }

  // "aTime" must have an incremental value
  void Write(const TTime& aTime, const TData& aData) {
    if (OpenFile()) {
      ReadIndex();
      WriteIndex(aTime, aData);
      CloseFile();
    }
  }

private:

  FS& FileSystem() const {
    return *mFileSystem;
  }

  size_t GetFileSize() const {
    return mNumberOfDataBlocks * (sizeof(TTime) + sizeof(TData));
  }

  size_t GetPosition(uint32_t aIndex) const {
    return aIndex * (sizeof(TTime) + sizeof(TData));
  }

  // allocates a complete new file and opens it for update
  void ResetAndOpenFile() {
    // ultimately clean up
    mFile.close();
    FileSystem().remove(mFileName.c_str());
    // create a new empty file for writing
    mFile = FileSystem().open(mFileName.c_str(), "w");
    if (mFile) {
      // initialize all bytes with zero
      size_t vPosition = GetFileSize();;
      while (vPosition--) mFile.write(0);
      mFile.flush();
      mFile.close();
      // for later operations, the file must be opened for update
      mFile = FileSystem().open(mFileName.c_str(), "r+b");
      // the file is empty - no need to "search" index in file
      mIndex = 0;
    }
  }

  bool OpenFile() {
    // open an existing file for update
    mFile = FileSystem().open(mFileName.c_str(), "r+b");
    // if file can't be opened, it probably doesn't exist
    if (!mFile) {
      ResetAndOpenFile();
    }
    // if the filesize does not match, we're resetting the file
    if (mFile && (mFile.size() != GetFileSize())) {
      ResetAndOpenFile();
    }
    // hopefully there is now a file open for update
    return mFile;
  }

  void ReadIndex() {
    // index is valid, if smaller than number of blocks
    if (mIndex < mNumberOfDataBlocks) return;
    // invalid index => need to search in file
    mIndex = 0;
    TTime vTime = 0;
    TTime vLastTime = 0;
    // search whole file when "time" jumps back
    while (mIndex < mNumberOfDataBlocks) {
      size_t vPosition = GetPosition(mIndex);
      if (mFile.seek(vPosition)) {
        if (Read(mFile, vTime)) {
          // found free (unused) block
          if (vTime == 0) return;
          // found wrap-around block (ring closed)
          if (vTime < vLastTime) return;
          // remember current time for next check
          vLastTime = vTime;
        }
      }
      // iterate next
      ++mIndex;
    }
    // last block was newest
    mIndex = 0;
  }

  void WriteIndex(const TTime& aTime, const TData& aData) {
    size_t vPosition = GetPosition(mIndex++);
    if (mIndex >= mNumberOfDataBlocks) mIndex = 0;
    if (mFile.seek(vPosition)) {
      Write(mFile, aTime);
      Write(mFile, aData);
    }
  }

  void CloseFile() {
    mFile.flush();
    mFile.close();
  }

  template <typename T>
  static bool Read(File& aFile, T& aData) {
    return aFile.read(reinterpret_cast<uint8_t*>(&aData), sizeof(T)) == sizeof(T);
  }

  template <typename T>
  static bool Write(File& aFile, const T& aData) {
    return aFile.write(reinterpret_cast<uint8_t*>(const_cast<T*>(&aData)), sizeof(T)) == sizeof(T);
  }

  FS* mFileSystem;

  const String mFileName;
  const uint32_t mNumberOfDataBlocks;

  File mFile;
  uint32_t mIndex;

};