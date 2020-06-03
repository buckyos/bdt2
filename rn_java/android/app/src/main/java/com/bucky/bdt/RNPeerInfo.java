package com.bucky.bdt;

import androidx.annotation.NonNull;

import com.facebook.react.bridge.*;
import com.bucky.bdt.PeerConstInfo;
import com.bucky.bdt.PeerInfo;

public class RNPeerInfo extends ReactContextBaseJavaModule {
    public RNPeerInfo(ReactApplicationContext reactContext) {
        super(reactContext);
    }

    public static PeerInfo fromReadableMap(ReadableMap reader) {
        PeerInfo.Builder peerInfoBuilder = new PeerInfo.Builder(PeerInfo.Builder.flagsHasSecret | PeerInfo.Builder.flagsHasSnInfo | PeerInfo.Builder.flagsHasSnList);
        ReadableMap constInfoReader = reader.getMap("constInfo");
        if (constInfoReader == null) {
            return null;
        }
        peerInfoBuilder.setConstInfo(RNPeerConstInfo.fromReadableMap(constInfoReader));

        byte[] secret = reader.getString("secret").getBytes();
        if (secret != null) {
            peerInfoBuilder.setSecret(secret);
        }

        ReadableArray endpoints = reader.getArray("endpoints");
        if (endpoints != null) {
            for (int i = 0; i < endpoints.size(); i++) {
                peerInfoBuilder.addEndpoint(endpoints.getString(i));
            }
        }

        ReadableArray superNodes = reader.getArray("supernodes");
        if (superNodes != null) {
            for (int i = 0; i < superNodes.size(); i++) {
                if (superNodes.getType(i) == ReadableType.String) {
                    peerInfoBuilder.addSn(superNodes.getString(i).getBytes());
                } else {
                    PeerInfo snPeerInfo = RNPeerInfo.fromReadableMap(superNodes.getMap(i));
                    if (snPeerInfo != null) {
                        peerInfoBuilder.addSnInfo(snPeerInfo);
                        // <TODO> addSn
                    }
                }
            }
        }

        return peerInfoBuilder.finish();
    }

    @NonNull
    @Override
    public String getName() {
        return "BdtPeerInfo";
    }

    @Override
    public boolean canOverrideExistingModule() {
        return true;
    }
}
