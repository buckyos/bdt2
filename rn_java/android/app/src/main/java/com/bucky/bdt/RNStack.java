package com.bucky.bdt;

import android.util.SparseArray;

import javax.annotation.Nullable;
import java.util.HashMap;
import java.util.Map;
import com.facebook.react.bridge.*;
import com.bucky.bdt.Stack;

public class RNStack extends ReactContextBaseJavaModule {

    public RNStack(ReactApplicationContext reactContext) {
        super(reactContext);
    }

    @ReactMethod
    public void create(ReadableMap localPeerReader, ReadableArray initialPeersReader, Callback callback) {
        Stack stack = new Stack();
        int stackId = mNextId++;

        addStack(stackId, stack);

        if (localPeerReader == null) {
            callback.invoke(RNBdtResult.invalidParam);
            return;
        }

        PeerInfo localPeer = RNPeerInfo.fromReadableMap(localPeerReader);
        if (localPeer == null) {
            callback.invoke(RNBdtResult.invalidParam);
            return;
        }

        int initialPeerCount = 0;
        PeerInfo[] initialPeers;
        if (initialPeersReader == null) {
            initialPeers = new PeerInfo[0];
        } else {
            initialPeers = new PeerInfo[initialPeersReader.size()];
            for (int i = 0; i < initialPeersReader.size(); i++) {
                ReadableMap peerInfoReader = initialPeersReader.getMap(i);
                PeerInfo peerInfo = RNPeerInfo.fromReadableMap(peerInfoReader);
                if (peerInfo != null) {
                    initialPeers[initialPeerCount] = peerInfo;
                    initialPeerCount++;
                }
            }
        }

        RNStack rnStack = this;

        stack.open(localPeer, initialPeers, new Stack.EventListener() {
            @Override
            public void onOpen() {
                callback.invoke(RNBdtResult.success, stackId);
            }

            @Override
            public void onError(int error) {
                callback.invoke(error);
                removeStack(stackId);
            }
        });

        localPeer.finalize();
        for (PeerInfo peer: initialPeers) {
            peer.finalize();
        }
    }

    @ReactMethod
    public void destroy(int stackId) {
        Stack stack = getStack(stackId);
        if (stack != null) {
            stack.close();
            removeStack(stackId);
        }
    }

    @ReactMethod
    public void addStaticPeerInfo(int stackId, ReadableMap peerInfoReader) {
        Stack stack = getStack(stackId);
        if (stack == null) {
            return;
        }

        if (peerInfoReader == null) {
            return;
        }

        PeerInfo peerInfo = RNPeerInfo.fromReadableMap(peerInfoReader);
        if (peerInfo == null) {
            return;
        }

        // <TODO>
        // stack.addStaticPeerInfo(peerInfo);
        peerInfo.finalize();
    }

    @ReactMethod
    public void connectRemote(int stackId,
                              ReadableMap remotePeerReader,
                              int vport,
                              ReadableArray remoteSnListReader,
                              String question,
                              Callback onAnswer,
                              Callback onReturn,
                              Callback onConnect) {

        Stack stack = getStack(stackId);

        if (stack == null) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }

        RNConnection.connectRemote(stack, remotePeerReader, vport, remoteSnListReader, question, onReturn, onConnect, onAnswer);
    }

    @ReactMethod
    public void listenConnection(int stackId, int vport, Callback onReturn) {
        Stack stack = getStack(stackId);
        if (stack == null) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }
        RNConnection.listen(stack, vport, onReturn);
    }

    @ReactMethod
    public void acceptConnection(int stackId, int vport, Callback onReturn, Callback preAccept, Callback onError) {
        Stack stack = getStack(stackId);
        if (stack == null) {
            onReturn.invoke(RNBdtResult.invalidParam);
            return;
        }
        RNConnection.accept(stack, vport, onReturn, preAccept, onError);
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
        return "BdtStack";//名称
    }

    @Override
    public boolean canOverrideExistingModule() {
        return true;
    }

    static  private Stack getStack(Integer id) {
        Stack stack;
        synchronized (mStackMap) {
            stack = mStackMap.get(id);
        }
        return stack;
    }

    static private void addStack(Integer id, Stack stack) {
        synchronized (mStackMap) {
            mStackMap.put(id, stack);
        }
    }

    static private void removeStack(Integer id) {
        synchronized (mStackMap) {
            mStackMap.remove(id);
        }
    }

    static private SparseArray<Stack> mStackMap;
    static private Integer mNextId;

    static {
        mStackMap = new SparseArray<Stack>();
        mNextId = 1;
    }
}
