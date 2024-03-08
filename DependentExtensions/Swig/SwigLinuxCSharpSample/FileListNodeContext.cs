/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace RakNet {

using System;
using System.Runtime.InteropServices;

public class FileListNodeContext : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal FileListNodeContext(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(FileListNodeContext obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~FileListNodeContext() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          RakNetPINVOKE.delete_FileListNodeContext(swigCPtr);
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

  public FileListNodeContext() : this(RakNetPINVOKE.new_FileListNodeContext__SWIG_0(), true) {
  }

  public FileListNodeContext(byte o, uint f) : this(RakNetPINVOKE.new_FileListNodeContext__SWIG_1(o, f), true) {
  }

  public byte op {
    set {
      RakNetPINVOKE.FileListNodeContext_op_set(swigCPtr, value);
    } 
    get {
      byte ret = RakNetPINVOKE.FileListNodeContext_op_get(swigCPtr);
      return ret;
    } 
  }

  public uint fileId {
    set {
      RakNetPINVOKE.FileListNodeContext_fileId_set(swigCPtr, value);
    } 
    get {
      uint ret = RakNetPINVOKE.FileListNodeContext_fileId_get(swigCPtr);
      return ret;
    } 
  }

}

}
