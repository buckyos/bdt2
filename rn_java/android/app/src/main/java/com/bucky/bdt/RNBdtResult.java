package com.bucky.bdt;

import com.facebook.react.bridge.Callback;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.bridge.ReadableMap;

import java.util.HashMap;
import java.util.Map;

import javax.annotation.Nullable;

public class RNBdtResult extends ReactContextBaseJavaModule {
    public static final int success = 0;
    public static final int failed = 1;
    public static final int invalidParam = 2;
    public static final int pending = 6;
    public static final int invalidState = 11;
    public static final int timeout = 29;

    public RNBdtResult(ReactApplicationContext reactContext) {
        super(reactContext);
    }

    @Nullable
    @Override
    public Map<String, Object> getConstants() {
        //让js那边能够使用这些常量
        Map<String,Object> constants = new HashMap<>();
        constants.put("success", 0);
        constants.put("failed", 1);
        constants.put("invalidParam", 2);
        constants.put("pending", 6);
        constants.put("invalidState", 11);
        constants.put("timeout", 29);
        return constants;
    }

    @Override
    public String getName() {
        return "BdtResult";//名称
    }

    @Override
    public boolean canOverrideExistingModule() {
        return true;
    }
}
