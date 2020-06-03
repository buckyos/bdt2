package com.bucky.bdt;

import android.telecom.Call;
import android.util.SparseArray;

import com.facebook.react.bridge.*;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.annotation.Nullable;

public class RNConnection extends ReactContextBaseJavaModule {

    public RNConnection(ReactApplicationContext reactContext) {
        super(reactContext);
    }

    public static void connectRemote(Stack stack,
                                     ReadableMap remotePeerReader,
                                     int vport,
                                     ReadableArray remoteSnListReader,
                                     String question,
                                     Callback onReturn,
                                     Callback onConnect,
                                     Callback onAnswer) {

        Integer connectionId = mNextId++;

        PeerConstInfo remotePeer = RNPeerConstInfo.fromReadableMap(remotePeerReader);
        if (remotePeer == null) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }

        byte[][] remoteSnList;
        if (remoteSnListReader != null && remoteSnListReader.size() > 0) {
            remoteSnList = new byte[remoteSnListReader.size()][RNPeerConstInfo.peeridLength];
            for (int i = 0; i < remoteSnListReader.size(); i++) {
                remoteSnList[i] = remoteSnListReader.getString(i).getBytes();
                if (remoteSnList[i].length != RNPeerConstInfo.peeridLength) {
                    onReturn.invoke(RNBdtResult.invalidParam);
                    return;
                }
            }
        } else {
            remoteSnList = new byte[0][0];
        }

        AtomicBoolean connectionAdded = new AtomicBoolean(false);

        Connection connection = Connection.connectRemote(stack, remotePeer, (short)vport, remoteSnList, question.getBytes(), new Connection.ConnectListener() {

            @Override
            public void onAnswer(byte[] answer) {
                onAnswer.invoke(answer);
            }

            @Override
            public void onEstablish() {
                if (!connectionAdded.compareAndSet(false, true)) {
                    addConnection(connectionId, this.m_connection);
                }

                onConnect.invoke(RNBdtResult.success, connectionId.intValue());
            }

            @Override
            public void onError(int error) {
                this.m_connection.reset();
                this.m_connection.finalize();
                onConnect.invoke(error);
            }
        });

        if (!connectionAdded.compareAndSet(false, true)) {
            addConnection(connectionId, connection);
        }

        onReturn.invoke(RNBdtResult.pending, connectionId.intValue());
    }

    @ReactMethod
    public void send(int connectId, String buffer, Callback onReturn, Callback onSent) {
        if (buffer.length() < 0) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }

        if (buffer.length() == 0) {
            onReturn.invoke(RNBdtResult.success, 0);
            return;
        }

        Connection connection = getConnection(connectId);
        if (connection == null) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }

        int ret = connection.send(new Connection.SendListener(buffer.getBytes()) {
            @Override
            public void onSent(int error, long sent) {
                onSent.invoke(error, sent);
            }
        });

        onReturn.invoke(ret);
    }

    @ReactMethod
    public void recv(int connectionId, String buffer, Callback onReturn, Callback onRecv) {
        if (buffer.length() <= 0) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }

        Connection connection = getConnection(connectionId);
        if (connection == null) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }

        int ret = connection.recv(new Connection.RecvListener(buffer.getBytes()) {
            @Override
            public void onRecv(long recv) {
                onRecv.invoke(buffer, recv);
            }
        });

        onReturn.invoke(ret);
    }

    @ReactMethod
    public void close(int connectionId, Callback onReturn) {
        Connection connection = getConnection(connectionId);
        int ret = RNBdtResult.invalidParam;
        if (connection != null) {
            removeConnection(connectionId);
            ret = connection.close();
            connection.finalize();
        }
        onReturn.invoke(ret);
    }

    @ReactMethod
    public void reset(int connectionId) {
        Connection connection = getConnection(connectionId);
        if (connection != null) {
            removeConnection(connectionId);
            connection.finalize();
            connection.reset();
        }
    }

    public static void listen(Stack stack, int vport, Callback onReturn) {
        int ret = Connection.listen(stack, (short)vport);
        onReturn.invoke(ret);
    }

    public static void accept(Stack stack, int vport, Callback onReturn, Callback preAccept, Callback onError) {
        Integer nextAcceptId = mNextId++;
        int ret = Connection.accept(stack, (short) vport, new Connection.PreAcceptListener() {
            @Override
            public void onConnection(Connection conn, byte[] question) {
                addConnection(nextAcceptId, conn);
                preAccept.invoke(nextAcceptId, question);
            }

            @Override
            public void onError(int error) {
                onError.invoke(error);
            }
        });

        onReturn.invoke(ret);
    }

    @ReactMethod
    public void confirm(int connectionId, String buffer, Callback onReturn, Callback onConnect) {
        if (buffer.length() <= 0) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }

        Connection connection = getConnection(connectionId);
        if (connection == null) {
            onReturn.invoke((RNBdtResult.invalidParam));
            return;
        }

        int ret = connection.confirm(buffer.getBytes(), new Connection.AcceptListener() {
            @Override
            public void onEstablish() {
                onConnect.invoke(RNBdtResult.success, connectionId);
            }

            @Override
            public void onError(int error) {
                onConnect.invoke(error);
            }
        });
        onReturn.invoke(ret);
    }

    @Nullable
    @Override
    public Map<String, Object> getConstants() {
        //让js那边能够使用这些常量
        Map<String,Object> constants = new HashMap<>();
        return constants;
    }

    @Override
    public String getName() {
        return "BdtConnection";//名称
    }

    @Override
    public boolean canOverrideExistingModule() {
        return true;
    }

    static private Connection getConnection(Integer id) {
        Connection connection;
        synchronized (mConnectionMap) {
            connection = mConnectionMap.get(id);
        }
        return connection;
    }

    static private void addConnection(Integer id, Connection connection) {
        synchronized (mConnectionMap) {
            mConnectionMap.put(id, connection);
        }
    }

    static private void removeConnection(Integer id) {
        synchronized (mConnectionMap) {
            mConnectionMap.remove(id);
        }
    }

    static private SparseArray<Connection> mConnectionMap;
    static private Integer mNextId;

    static {
        mConnectionMap = new SparseArray<Connection>();
        mNextId = 1;
    }
}
