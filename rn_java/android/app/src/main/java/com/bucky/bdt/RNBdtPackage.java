package com.bucky.bdt;

import com.facebook.react.ReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.uimanager.ViewManager;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class RNBdtPackage implements ReactPackage {
    //完成注入需要被js调用java方法
    @Override
    public List<NativeModule> createNativeModules(ReactApplicationContext reactContext) {
        List<NativeModule> modules = new ArrayList<>();
        modules.add(new RNStack(reactContext));
        modules.add(new RNConnection(reactContext));
        modules.add(new RNPeerConstInfo(reactContext));
        modules.add(new RNPeerInfo(reactContext));
        modules.add(new RNBdtResult(reactContext));
        return modules;
    }

    @Override
    public List<ViewManager> createViewManagers(ReactApplicationContext reactContext) {
        return Collections.emptyList();
    }
}
