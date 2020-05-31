package com.bucky.bdt;

public class Connection extends NativePointerObject {
    public abstract static class ConnectListener {
        protected Connection m_connection;
        public abstract void onAnswer(byte[] answer);
        public abstract void onEstablish();
        public abstract void onError(int error);
    }

    public static Connection connectRemote(
            Stack stack,
            PeerConstInfo remotePeer,
            short vport,
            byte[][] remoteSnList,
            byte[] question,
            ConnectListener cl
    )
    {
        Connection conn = new Connection();
        conn.connect(stack, remotePeer, vport, remoteSnList, question, cl);
        return conn;
    }
    private native int connect(
            Stack stack,
            PeerConstInfo remotePeer,
            short vport,
            byte[][] remoteSnList,
            byte[] question,
            ConnectListener cl);

    public static abstract class SendListener {
        SendListener(byte[] buffer) {
            this.m_buffer = buffer;
        }
        public abstract void onSent(int error, long sent);
        public byte[] getBuffer() {
            return this.m_buffer;
        }
        private final byte[] m_buffer;
    }
    public native int send(
            SendListener listener
    );

    public static abstract class RecvListener {
        RecvListener(byte[] buffer) {
            this.m_buffer = buffer;
        }
        public abstract void onRecv(long recv);
        public byte[] getBuffer() {
            return this.m_buffer;
        }
        private final byte[] m_buffer;
    }
    public native int recv(
        RecvListener listener
    );

    public native int close();
    public native void reset();

    public static native int listen(Stack stack, short vport);

    public interface PreAcceptListener {
        void onError(int error);
        void onConnection(Connection conn, byte[] question);
    }

    public interface AcceptListener {
        void onEstablish();
        void onError(int error);
    }
    public static native int accept(
            Stack stack,
            short vport,
            PreAcceptListener listener);

    public native int confirm(byte[] answer, AcceptListener listener);

    public void finalize() {
        this.releaseNative();
    }
    private native void releaseNative();
}
