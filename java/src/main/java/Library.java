/*
 * This Java source file was generated by the Gradle 'init' task.
 */

import java.nio.*;

public class Library {
  private ByteBuffer memory;
  static {
    //System.load("/home/jbapple/code/bloom/c/lib/libfilter.so");
    //System.loadLibrary("filter");
    System.loadLibrary("jni-bridge");
  }
  private native void doNothing();
  private native boolean Allocate(ByteBuffer bb, int bytes);
  private native boolean FindDetail(ByteBuffer bb, long hashval);
  public boolean Find(long hashval) { return FindDetail(memory, hashval); }
  private native void AddDetail(ByteBuffer bb, long hashval);
  public void Add(long hashval) { AddDetail(memory, hashval); }
  private native boolean Deallocate(ByteBuffer bb);
  public boolean someLibraryMethod() { return true; }
  public Library(int bytes) {
    this.memory = ByteBuffer.allocateDirect(24);
    Allocate(memory, bytes);
  }
  public void finalize() {
    Deallocate(memory);
  }
}
