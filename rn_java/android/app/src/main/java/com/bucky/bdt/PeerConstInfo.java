package com.bucky.bdt;

public class PeerConstInfo {
    public byte continent;
    public byte country;
    public byte carrier;
    public short city;
    public byte inner;
    public String deviceId;
    public byte[] publicKey;
    public native byte[] peerid();
}
