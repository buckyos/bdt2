package com.bucky.bdt;

public class Stack extends NativePointerObject {
    public interface EventListener {
        void onOpen();
        void onError(int error);
    }


    public native int open(
            PeerInfo local,
            PeerInfo[] initialPeers,
            EventListener listener);

    public native void close();
}
