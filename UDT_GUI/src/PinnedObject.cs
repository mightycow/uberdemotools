using System;
using System.Runtime.InteropServices;


namespace Uber
{
    public class PinnedObject
    {
        private GCHandle _pinnedObjectHandle;
        private IntPtr _address = IntPtr.Zero;

        public IntPtr Address
        {
            get { return _address; }
        }

        public PinnedObject(object objectToPin)
        {
            _pinnedObjectHandle = GCHandle.Alloc(objectToPin, GCHandleType.Pinned);
            _address = _pinnedObjectHandle.AddrOfPinnedObject();
        }

        public void Free()
        {
            _pinnedObjectHandle.Free();
            _address = IntPtr.Zero;
        }
    }
}