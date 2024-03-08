/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.0
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace RakNet {

using System;
using System.Runtime.InteropServices;

public class UDPForwarder : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal UDPForwarder(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(UDPForwarder obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~UDPForwarder() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          RakNetPINVOKE.delete_UDPForwarder(swigCPtr);
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

  public UDPForwarder() : this(RakNetPINVOKE.new_UDPForwarder(), true) {
  }

  public void Startup() {
    RakNetPINVOKE.UDPForwarder_Startup(swigCPtr);
  }

  public void Shutdown() {
    RakNetPINVOKE.UDPForwarder_Shutdown(swigCPtr);
  }

  public void Update() {
    RakNetPINVOKE.UDPForwarder_Update(swigCPtr);
  }

  public void SetMaxForwardEntries(ushort maxEntries) {
    RakNetPINVOKE.UDPForwarder_SetMaxForwardEntries(swigCPtr, maxEntries);
  }

  public int GetMaxForwardEntries() {
    int ret = RakNetPINVOKE.UDPForwarder_GetMaxForwardEntries(swigCPtr);
    return ret;
  }

  public int GetUsedForwardEntries() {
    int ret = RakNetPINVOKE.UDPForwarder_GetUsedForwardEntries(swigCPtr);
    return ret;
  }

  public UDPForwarderResult StartForwarding(SystemAddress source, SystemAddress destination, uint timeoutOnNoDataMS, string forceHostAddress, out ushort forwardingPort, out uint forwardingSocket) {
    UDPForwarderResult ret = (UDPForwarderResult)RakNetPINVOKE.UDPForwarder_StartForwarding(swigCPtr, SystemAddress.getCPtr(source), SystemAddress.getCPtr(destination), timeoutOnNoDataMS, forceHostAddress, out forwardingPort, out forwardingSocket);
    if (RakNetPINVOKE.SWIGPendingException.Pending) throw RakNetPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public void StopForwarding(SystemAddress source, SystemAddress destination) {
    RakNetPINVOKE.UDPForwarder_StopForwarding(swigCPtr, SystemAddress.getCPtr(source), SystemAddress.getCPtr(destination));
    if (RakNetPINVOKE.SWIGPendingException.Pending) throw RakNetPINVOKE.SWIGPendingException.Retrieve();
  }

  public SimpleMutex threadOperationIncomingMutex {
    set {
      RakNetPINVOKE.UDPForwarder_threadOperationIncomingMutex_set(swigCPtr, SimpleMutex.getCPtr(value));
    } 
    get {
      IntPtr cPtr = RakNetPINVOKE.UDPForwarder_threadOperationIncomingMutex_get(swigCPtr);
      SimpleMutex ret = (cPtr == IntPtr.Zero) ? null : new SimpleMutex(cPtr, false);
      return ret;
    } 
  }

  public SimpleMutex threadOperationOutgoingMutex {
    set {
      RakNetPINVOKE.UDPForwarder_threadOperationOutgoingMutex_set(swigCPtr, SimpleMutex.getCPtr(value));
    } 
    get {
      IntPtr cPtr = RakNetPINVOKE.UDPForwarder_threadOperationOutgoingMutex_get(swigCPtr);
      SimpleMutex ret = (cPtr == IntPtr.Zero) ? null : new SimpleMutex(cPtr, false);
      return ret;
    } 
  }

  public void UpdateThreaded() {
    RakNetPINVOKE.UDPForwarder_UpdateThreaded(swigCPtr);
  }

  public UDPForwarderResult StartForwardingThreaded(SystemAddress source, SystemAddress destination, uint timeoutOnNoDataMS, string forceHostAddress, out ushort forwardingPort, out uint forwardingSocket) {
    UDPForwarderResult ret = (UDPForwarderResult)RakNetPINVOKE.UDPForwarder_StartForwardingThreaded(swigCPtr, SystemAddress.getCPtr(source), SystemAddress.getCPtr(destination), timeoutOnNoDataMS, forceHostAddress, out forwardingPort, out forwardingSocket);
    if (RakNetPINVOKE.SWIGPendingException.Pending) throw RakNetPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public void StopForwardingThreaded(SystemAddress source, SystemAddress destination) {
    RakNetPINVOKE.UDPForwarder_StopForwardingThreaded(swigCPtr, SystemAddress.getCPtr(source), SystemAddress.getCPtr(destination));
    if (RakNetPINVOKE.SWIGPendingException.Pending) throw RakNetPINVOKE.SWIGPendingException.Retrieve();
  }

  public ushort maxForwardEntries {
    set {
      RakNetPINVOKE.UDPForwarder_maxForwardEntries_set(swigCPtr, value);
    } 
    get {
      ushort ret = RakNetPINVOKE.UDPForwarder_maxForwardEntries_get(swigCPtr);
      return ret;
    } 
  }

  public bool isRunning {
    set {
      RakNetPINVOKE.UDPForwarder_isRunning_set(swigCPtr, value);
    } 
    get {
      bool ret = RakNetPINVOKE.UDPForwarder_isRunning_get(swigCPtr);
      return ret;
    } 
  }

  public bool threadRunning {
    set {
      RakNetPINVOKE.UDPForwarder_threadRunning_set(swigCPtr, value);
    } 
    get {
      bool ret = RakNetPINVOKE.UDPForwarder_threadRunning_get(swigCPtr);
      return ret;
    } 
  }

}

}
