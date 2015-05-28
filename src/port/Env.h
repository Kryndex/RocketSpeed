// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
// An Env is an interface used by the rocketspeed implementation to access
// operating system functionality like the filesystem etc.  Callers
// may wish to provide a custom Env object when using rocketspeed to
// port it to a different platform
//
// All Env implementations are safe for concurrent access from
// multiple threads without any external synchronization.
#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include "include/Status.h"
#include "include/Logger.h"
#include "src/util/common/base_env.h"
#include "src/util/common/env_options.h"

namespace rocketspeed {

class FileLock;
class RandomAccessFile;
class Slice;
class WritableFile;
class RandomRWFile;
class Directory;
class Connection;

using std::unique_ptr;
using std::shared_ptr;

class Env : public BaseEnv {
 public:
  Env() { }
  virtual ~Env();

  // Install signal handlers required by server side code.
  static void InstallSignalHandlers();

  // Return a default environment suitable for the current operating
  // system.  Sophisticated users may wish to provide their own Env
  // implementation instead of relying on this default environment.
  //
  // The result of Default() belongs to rocketspeed and must never be deleted.
  static Env* Default();

  // Create a brand new random access read-only file with the
  // specified name.  On success, stores a pointer to the new file in
  // *result and returns OK.  On failure stores nullptr in *result and
  // returns non-OK.  If the file does not exist, returns a non-OK
  // status.
  //
  // The returned file may be concurrently accessed by multiple threads.
  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options)
                                     = 0;

  // Create an object that writes to a new file with the specified
  // name.  Deletes any existing file with the same name and creates a
  // new file.  On success, stores a pointer to the new file in
  // *result and returns OK.  On failure stores nullptr in *result and
  // returns non-OK.
  //
  // The returned file will only be accessed by one thread at a time.
  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& options) = 0;

  // Create an object that both reads and writes to a file on
  // specified offsets (random access). If file already exists,
  // does not overwrite it. On success, stores a pointer to the
  // new file in *result and returns OK. On failure stores nullptr
  // in *result and returns non-OK.
  virtual Status NewRandomRWFile(const std::string& fname,
                                 unique_ptr<RandomRWFile>* result,
                                 const EnvOptions& options) = 0;

  // Create an object that represents a directory. Will fail if directory
  // doesn't exist. If the directory exists, it will open the directory
  // and create a new Directory object.
  //
  // On success, stores a pointer to the new Directory in
  // *result and returns OK. On failure stores nullptr in *result and
  // returns non-OK.
  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) = 0;

  // Returns true iff the named file exists.
  virtual bool FileExists(const std::string& fname) = 0;

  // Store in *result the names of the children of the specified directory.
  // The names are relative to "dir".
  // Original contents of *results are dropped.
  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) = 0;

  // Delete the named file.
  virtual Status DeleteFile(const std::string& fname) = 0;

  // Create the specified directory. Returns error if directory exists.
  virtual Status CreateDir(const std::string& dirname) = 0;

  // Creates directory if missing. Return Ok if it exists, or successful in
  // Creating.
  virtual Status CreateDirIfMissing(const std::string& dirname) = 0;

  // Delete the specified empty directory.
  virtual Status DeleteDir(const std::string& dirname) = 0;

  // Delete the specified directory and all its contents.
  virtual Status DeleteDirRecursive(const std::string& dirname) = 0;

  // Store the size of fname in *file_size.
  virtual Status GetFileSize(const std::string& fname, uint64_t* file_size) = 0;

  // Store the last modification time of fname in *file_mtime.
  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* file_mtime) = 0;
  // Rename file src to target.
  virtual Status RenameFile(const std::string& src,
                            const std::string& target) = 0;

  // Lock the specified file.  On failure, stores nullptr in
  // *lock and returns non-OK.
  //
  // On success, stores a pointer to the object that represents the
  // acquired lock in *lock and returns OK.  The caller should call
  // UnlockFile(*lock) to release the lock.  If the process exits,
  // the lock will be automatically released.
  //
  // If somebody else already holds the lock, finishes immediately
  // with a failure.  I.e., this call does not wait for existing locks
  // to go away.
  //
  // May create the named file if it does not already exist.
  virtual Status LockFile(const std::string& fname, FileLock** lock) = 0;

  // Release the lock acquired by a previous successful call to LockFile.
  // REQUIRES: lock was returned by a successful LockFile() call
  // REQUIRES: lock has not already been unlocked.
  virtual Status UnlockFile(FileLock* lock) = 0;

  // Priority for scheduling job in thread pool
  enum Priority { LOW, HIGH, TOTAL };

  // Priority for scheduling job in thread pool
  enum IOPriority {
    IO_LOW = 0,
    IO_HIGH = 1,
    IO_TOTAL = 2
  };

  // An identifier for a thread.
  typedef uint64_t ThreadId;

  // Arrange to run "(*function)(arg)" once in a background thread, in
  // the thread pool specified by pri. By default, jobs go to the 'LOW'
  // priority thread pool.

  // "function" may run in an unspecified thread.  Multiple functions
  // added to the same Env may run concurrently in different threads.
  // I.e., the caller may not assume that background work items are
  // serialized.
  virtual void Schedule(
      void (*function)(void* arg),
      void* arg,
      Priority pri = LOW) = 0;

  // Start a new thread, invoking "function(arg)" within the new thread.
  // When "function(arg)" returns, the thread will be destroyed.
  // Returns an identifier for the thread that is created.
  virtual ThreadId StartThread(void (*function)(void* arg), void* arg,
                               const std::string& thread_name = "") = 0;

  // Start a new thread, invoking an std::function.
  virtual ThreadId StartThread(std::function<void()> f,
                               const std::string& thread_name = "") = 0;

  // Gets the thread ID for the current thread.
  virtual ThreadId GetCurrentThreadId() const = 0;

  // Waits for the specified thread to exit
  virtual void WaitForJoin(ThreadId tid) = 0;

  // Wait for all threads started by StartThread to terminate.
  virtual void WaitForJoin()  = 0;

  // The number of threads created so far
  virtual int GetNumberOfThreads() const = 0;

  // Get thread pool queue length for specific thread pool.
  virtual unsigned int GetThreadPoolQueueLen(Priority pri = LOW) const {
    return 0;
  }

  // *path is set to a temporary directory that can be used for testing. It may
  // or many not have just been created. The directory may or may not differ
  // between runs of the same process, but subsequent calls will return the
  // same directory.
  virtual Status GetTestDirectory(std::string* path) = 0;

  // Create and return a log file for storing informational messages.
  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) = 0;

  // Create and return a logger that writes to the standard error stream.
  virtual Status StdErrLogger(shared_ptr<Logger>* result) = 0;

  // Returns the number of micro-seconds since some fixed point in time. Only
  // useful for computing deltas of time.
  virtual uint64_t NowMicros() = 0;

  // Returns the number of nano-seconds since some fixed point in time. Only
  // useful for computing deltas of time in one run.
  // Default implementation simply relies on NowMicros
  virtual uint64_t NowNanos() {
    return NowMicros() * 1000;
  }

  // Sleep/delay the thread for the perscribed number of micro-seconds.
  virtual void SleepForMicroseconds(int micros) = 0;

  // Get the current host name.
  virtual Status GetHostName(char* name, uint64_t len) = 0;

  // Get the number of seconds since the Epoch, 1970-01-01 00:00:00 (UTC).
  virtual Status GetCurrentTime(int64_t* unix_time) = 0;

  // Get full name of current working directory
  virtual Status GetWorkingDirectory(std::string* output_path) = 0;

  // Set name of current working directory
  virtual Status ChangeWorkingDirectory(const std::string& path) = 0;

  // The number of background worker threads of a specific thread pool
  // for this environment. 'LOW' is the default pool.
  // default number: 1
  virtual void SetBackgroundThreads(int number, Priority pri = LOW) = 0;

  // Converts seconds-since-Jan-01-1970 to a printable string
  virtual std::string TimeToString(uint64_t time) = 0;

  // Generates a unique id that can be used to identify a db
  virtual std::string GenerateUniqueId();

  // Create a brand new Connection to the remote server.
  // The returned Connection object is not-thread safe.
  virtual Status NewConnection(const std::string& hostname,
                               int port,
                               bool blocking,
                               unique_ptr<Connection>* result,
                               const EnvOptions& options) const
                               = 0;
  // Number of CPUs on this machine
  virtual unsigned int GetNumberOfCpus() {
    return 1;
  }

 private:
  // No copying allowed
  Env(const Env&);
  void operator=(const Env&);
};

// A file abstraction for randomly reading the contents of a file.
class RandomAccessFile {
 public:
  RandomAccessFile() { }
  virtual ~RandomAccessFile();

  // Read up to "n" bytes from the file starting at "offset".
  // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
  // to the data that was read (including if fewer than "n" bytes were
  // successfully read).  May set "*result" to point at data in
  // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
  // "*result" is used.  If an error was encountered, returns a non-OK
  // status.
  //
  // Safe for concurrent use by multiple threads.
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const = 0;

  // Tries to get an unique ID for this file that will be the same each time
  // the file is opened (and will stay the same while the file is open).
  // Furthermore, it tries to make this ID at most "max_size" bytes. If such an
  // ID can be created this function returns the length of the ID and places it
  // in "id"; otherwise, this function returns 0, in which case "id"
  // may not have been modified.
  //
  // This function guarantees, for IDs from a given environment, two unique ids
  // cannot be made equal to eachother by adding arbitrary bytes to one of
  // them. That is, no unique ID is the prefix of another.
  //
  // This function guarantees that the returned ID will not be interpretable as
  // a single varint.
  //
  // Note: these IDs are only valid for the duration of the process.
  virtual size_t GetUniqueId(char* id, size_t max_size) const {
    return 0; // Default implementation to prevent issues with backwards
              // compatibility.
  };


  enum AccessPattern { NORMAL, RANDOM, SEQUENTIAL, WILLNEED, DONTNEED };

  virtual void Hint(AccessPattern pattern) {}

  // Remove any kind of caching of data from the offset to offset+length
  // of this file. If the length is 0, then it refers to the end of file.
  // If the system is not caching the file contents, then this is a noop.
  virtual Status InvalidateCache(size_t offset, size_t length) {
    return Status::NotSupported("InvalidateCache not supported.");
  }
};

// A file abstraction for sequential writing.  The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
class WritableFile {
 public:
  WritableFile()
    : last_preallocated_block_(0),
      preallocation_block_size_(0),
      io_priority_(Env::IO_TOTAL) {
  }
  virtual ~WritableFile();

  virtual Status Append(const Slice& data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0; // sync data

  /*
   * Sync data and/or metadata as well.
   * By default, sync only data.
   * Override this method for environments where we need to sync
   * metadata as well.
   */
  virtual Status Fsync() {
    return Sync();
  }

  /*
   * Change the priority in rate limiter if rate limiting is enabled.
   * If rate limiting is not enabled, this call has no effect.
   */
  virtual void SetIOPriority(Env::IOPriority pri) {
    io_priority_ = pri;
  }

  /*
   * Get the size of valid data in the file.
   */
  virtual uint64_t GetFileSize() {
    return 0;
  }

  /*
   * Get and set the default pre-allocation block size for writes to
   * this file.  If non-zero, then Allocate will be used to extend the
   * underlying storage of a file (generally via fallocate) if the Env
   * instance supports it.
   */
  void SetPreallocationBlockSize(size_t size) {
    preallocation_block_size_ = size;
  }

  virtual void GetPreallocationStatus(size_t* block_size,
                                      size_t* last_allocated_block) {
    *last_allocated_block = last_preallocated_block_;
    *block_size = preallocation_block_size_;
  }

  // For documentation, refer to RandomAccessFile::GetUniqueId()
  virtual size_t GetUniqueId(char* id, size_t max_size) const {
    return 0; // Default implementation to prevent issues with backwards
  }

  // Remove any kind of caching of data from the offset to offset+length
  // of this file. If the length is 0, then it refers to the end of file.
  // If the system is not caching the file contents, then this is a noop.
  // This call has no effect on dirty pages in the cache.
  virtual Status InvalidateCache(size_t offset, size_t length) {
    return Status::NotSupported("InvalidateCache not supported.");
  }

 protected:
  // PrepareWrite performs any necessary preparation for a write
  // before the write actually occurs.  This allows for pre-allocation
  // of space on devices where it can result in less file
  // fragmentation and/or less waste from over-zealous filesystem
  // pre-allocation.
  void PrepareWrite(size_t offset, size_t len) {
    if (preallocation_block_size_ == 0) {
      return;
    }
    // If this write would cross one or more preallocation blocks,
    // determine what the last preallocation block necesessary to
    // cover this write would be and Allocate to that point.
    const auto block_size = preallocation_block_size_;
    size_t new_last_preallocated_block =
      (offset + len + block_size - 1) / block_size;
    if (new_last_preallocated_block > last_preallocated_block_) {
      size_t num_spanned_blocks =
        new_last_preallocated_block - last_preallocated_block_;
      Allocate(static_cast<off_t>(block_size * last_preallocated_block_),
               static_cast<off_t>(block_size * num_spanned_blocks));
      last_preallocated_block_ = new_last_preallocated_block;
    }
  }

  /*
   * Pre-allocate space for a file.
   */
  virtual Status Allocate(off_t offset, off_t len) {
    return Status::OK();
  }

  // Sync a file range with disk.
  // offset is the starting byte of the file range to be synchronized.
  // nbytes specifies the length of the range to be synchronized.
  // This asks the OS to initiate flushing the cached data to disk,
  // without waiting for completion.
  // Default implementation does nothing.
  virtual Status RangeSync(off_t offset, off_t nbytes) {
    return Status::OK();
  }

 private:
  size_t last_preallocated_block_;
  size_t preallocation_block_size_;
  // No copying allowed
  WritableFile(const WritableFile&);
  void operator=(const WritableFile&);

 protected:
  Env::IOPriority io_priority_;
};

// A file abstraction for random reading and writing.
class RandomRWFile {
 public:
  RandomRWFile() {}
  virtual ~RandomRWFile() {}

  // Write data from Slice data to file starting from offset
  // Returns IOError on failure, but does not guarantee
  // atomicity of a write.  Returns OK status on success.
  //
  // Safe for concurrent use.
  virtual Status Write(uint64_t offset, const Slice& data) = 0;
  // Read up to "n" bytes from the file starting at "offset".
  // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
  // to the data that was read (including if fewer than "n" bytes were
  // successfully read).  May set "*result" to point at data in
  // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
  // "*result" is used.  If an error was encountered, returns a non-OK
  // status.
  //
  // Safe for concurrent use by multiple threads.
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const = 0;
  virtual Status Close() = 0; // closes the file
  virtual Status Sync() = 0; // sync data

  /*
   * Sync data and/or metadata as well.
   * By default, sync only data.
   * Override this method for environments where we need to sync
   * metadata as well.
   */
  virtual Status Fsync() {
    return Sync();
  }

  /*
   * Pre-allocate space for a file.
   */
  virtual Status Allocate(off_t offset, off_t len) {
    return Status::OK();
  }

 private:
  // No copying allowed
  RandomRWFile(const RandomRWFile&);
  void operator=(const RandomRWFile&);
};

// Directory object represents collection of files and implements
// filesystem operations that can be executed on directories.
class Directory {
 public:
  virtual ~Directory() {}
  // Fsync directory
  virtual Status Fsync() = 0;
};

// Identifies a locked file.
class FileLock {
 public:
  FileLock() { }
  virtual ~FileLock();
 private:
  // No copying allowed
  FileLock(const FileLock&);
  void operator=(const FileLock&);
};

// Identifies a connection to a remote server
class Connection {
 public:
  Connection() {}

  // Create a connection to the specified hostname and port
  Connection(const std::string& hostname, int port);

  // Write the data to the connection
  virtual Status Send(const Slice& data) = 0;

  virtual Status Receive(char* buffer, ssize_t* sz) = 0;

  virtual ~Connection() {}
};

// A utility routine: write "data" to the named file.
extern Status WriteStringToFile(Env* env, const Slice& data,
                                const std::string& fname,
                                bool should_sync = false);

// A utility routine: read contents of named file into *data
extern Status ReadFileToString(Env* env, const std::string& fname,
                               std::string* data);

// An implementation of Env that forwards all calls to another Env.
// May be useful to clients who wish to override just part of the
// functionality of another Env.
class EnvWrapper : public Env {
 public:
  // Initialize an EnvWrapper that delegates all calls to *t
  explicit EnvWrapper(Env* t) : target_(t) { }
  virtual ~EnvWrapper();

  // Return the target to which this Env forwards all calls
  Env* target() const { return target_; }

  // The following text is boilerplate that forwards all methods to target()
  Status NewSequentialFile(const std::string& f,
                           unique_ptr<SequentialFile>* r,
                           const EnvOptions& options) {
    return target_->NewSequentialFile(f, r, options);
  }
  Status NewRandomAccessFile(const std::string& f,
                             unique_ptr<RandomAccessFile>* r,
                             const EnvOptions& options) {
    return target_->NewRandomAccessFile(f, r, options);
  }
  Status NewWritableFile(const std::string& f, unique_ptr<WritableFile>* r,
                         const EnvOptions& options) {
    return target_->NewWritableFile(f, r, options);
  }
  Status NewRandomRWFile(const std::string& f, unique_ptr<RandomRWFile>* r,
                         const EnvOptions& options) {
    return target_->NewRandomRWFile(f, r, options);
  }
  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) {
    return target_->NewDirectory(name, result);
  }
  bool FileExists(const std::string& f) { return target_->FileExists(f); }
  Status GetChildren(const std::string& dir, std::vector<std::string>* r) {
    return target_->GetChildren(dir, r);
  }
  Status DeleteFile(const std::string& f) { return target_->DeleteFile(f); }
  Status CreateDir(const std::string& d) { return target_->CreateDir(d); }
  Status CreateDirIfMissing(const std::string& d) {
    return target_->CreateDirIfMissing(d);
  }
  Status DeleteDir(const std::string& d) { return target_->DeleteDir(d); }
  Status GetFileSize(const std::string& f, uint64_t* s) {
    return target_->GetFileSize(f, s);
  }

  Status GetFileModificationTime(const std::string& fname,
                                 uint64_t* file_mtime) {
    return target_->GetFileModificationTime(fname, file_mtime);
  }

  Status RenameFile(const std::string& s, const std::string& t) {
    return target_->RenameFile(s, t);
  }
  Status LockFile(const std::string& f, FileLock** l) {
    return target_->LockFile(f, l);
  }
  Status UnlockFile(FileLock* l) { return target_->UnlockFile(l); }
  void Schedule(void (*f)(void*), void* a, Priority pri) {
    return target_->Schedule(f, a, pri);
  }
  ThreadId StartThread(void (*f)(void*), void* a, const std::string& n) {
    return target_->StartThread(f, a, n);
  }
  ThreadId StartThread(std::function<void()> f, const std::string& n) {
    return target_->StartThread(std::move(f), n);
  }
  ThreadId GetCurrentThreadId() const {
    return target_->GetCurrentThreadId();
  }
  void WaitForJoin() { return target_->WaitForJoin(); }
  void WaitForJoin(ThreadId tid) { return target_->WaitForJoin(tid); }
  virtual int GetNumberOfThreads() const {
    return target_->GetNumberOfThreads();
  }

  virtual unsigned int GetThreadPoolQueueLen(Priority pri = LOW) const {
    return target_->GetThreadPoolQueueLen(pri);
  }
  virtual Status GetTestDirectory(std::string* path) {
    return target_->GetTestDirectory(path);
  }
  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) {
    return target_->NewLogger(fname, result);
  }
  uint64_t NowMicros() {
    return target_->NowMicros();
  }
  void SleepForMicroseconds(int micros) {
    target_->SleepForMicroseconds(micros);
  }
  Status GetHostName(char* name, uint64_t len) {
    return target_->GetHostName(name, len);
  }
  Status GetCurrentTime(int64_t* unix_time) {
    return target_->GetCurrentTime(unix_time);
  }
  Status GetWorkingDirectory(std::string* output_path) {
    return target_->GetWorkingDirectory(output_path);
  }
  Status ChangeWorkingDirectory(const std::string& path) {
    return target_->ChangeWorkingDirectory(path);
  }
  void SetBackgroundThreads(int num, Priority pri) {
    return target_->SetBackgroundThreads(num, pri);
  }
  std::string TimeToString(uint64_t time) {
    return target_->TimeToString(time);
  }
  Status NewConnection(const std::string& f,
                       const int p,
                       bool blocking,
                       unique_ptr<Connection>* r,
                       const EnvOptions& options) const {
    return target_->NewConnection(f, p, blocking, r, options);
  }

 private:
  Env* target_;
};

// Returns a new environment that stores its data in memory and delegates
// all non-file-storage tasks to base_env. The caller must delete the result
// when it is no longer needed.
// *base_env must remain live while the result is in use.
Env* NewMemEnv(Env* base_env);

}  // namespace rocketspeed
