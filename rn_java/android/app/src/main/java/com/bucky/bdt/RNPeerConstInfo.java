package com.bucky.bdt;

import android.telecom.Call;

import androidx.annotation.NonNull;

import com.facebook.react.bridge.*;

public class RNPeerConstInfo extends ReactContextBaseJavaModule {
    static public final int peeridLength = 32;

    public RNPeerConstInfo(ReactApplicationContext reactContext) {
        super((reactContext));
    }

    public static PeerConstInfo fromReadableMap(ReadableMap reader) {
        if (reader == null) {
            return null;
        }
        PeerConstInfo constInfo = new PeerConstInfo();
        constInfo.continent = (byte)reader.getInt("continent");
        constInfo.country = (byte)reader.getInt("country");
        constInfo.carrier = (byte)reader.getInt("carrier");
        constInfo.city = (short)reader.getInt("city");
        constInfo.inner = (byte)reader.getInt("inner");
        constInfo.deviceId = reader.getString("deviceId");
        constInfo.publicKey = reader.getString("publicKey").getBytes();

        return constInfo;
    }

    @ReactMethod
    public void toPeerid(ReadableMap constInfoReader, Callback callback) {
        PeerConstInfo constInfo = RNPeerConstInfo.fromReadableMap(constInfoReader);
        if (constInfo == null) {
            callback.invoke(null);
        }
        callback.invoke(new String(constInfo.peerid()));
    }

    @NonNull
    @Override
    public String getName() {
        return "BdtPeerConstInfo";
    }

    @Override
    public boolean canOverrideExistingModule() {
        return true;
    }
}
