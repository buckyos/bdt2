package com.bucky.bdt;

public class PeerInfo extends NativePointerObject {
    static class Builder extends NativePointerObject {
        public Builder(int flags) {
            this.init(flags);
        }
        static public final int flagsHasSecret = (1<<1);
        static public final int flagsHasSnList = (1<<2);
        static public final int flagsHasSnInfo = (1<<3);

        private native void init(int flags);
        public native Builder setConstInfo(PeerConstInfo constInfo);
        public native Builder setSecret(byte[] secret);
        public native Builder addEndpoint(String ep);
        public native Builder addSn(byte[] snPeerid);
        public native Builder addSnInfo(PeerInfo snInfo);
        public native PeerInfo finish();
    }

    public void finalize() {
        this.releaseNative();
    }
    private native void releaseNative();
}
