package com.bucky.bdt;

import android.content.Context;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
    static {
        System.loadLibrary("bdt2-jni");
    }
    @Test
    public void useAppContext() {
        // Context of the app under test.
        Context appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();

        PeerInfo localPeer = null;
        try {
            byte[] publicKey = hexToByte("30819f300d06092a864886f70d010101050003818d0030818902818100bf6632cca0dd6822d67ab9acce0eeeafb64629be3ea11624542c9f1471077d34adf8896913b4119bba8b11e353999fd9ee15f5e1a5d1abd67fef6fe4b8f45b968c388635d54a82e668531fd6b4b1c9c5ea1fd70551af170b4380d750c5a14406bfa2ba087fa1e1ec833058aa12dd2e0d4801305a6768477b15d6c27597a65f890203010001");
            byte[] secret = hexToByte("3082025c02010002818100bf6632cca0dd6822d67ab9acce0eeeafb64629be3ea11624542c9f1471077d34adf8896913b4119bba8b11e353999fd9ee15f5e1a5d1abd67fef6fe4b8f45b968c388635d54a82e668531fd6b4b1c9c5ea1fd70551af170b4380d750c5a14406bfa2ba087fa1e1ec833058aa12dd2e0d4801305a6768477b15d6c27597a65f89020301000102818000e943a4a5bf6817825de3346766bacc2b59fa28e5b36e9f8da708bad82ee8f1af4257a5206ae30a92c0c6bd0650dce9f4b0361374eea29acad120ff3dc22c0d714dda82d360a40b6356f18957a0cc1828fb131e5b22b88dd8be3daeb9093371d89002c879fe5831aca4558e83295fdef08c77f5fdc53d5f0cccad5442e1f861024100fa2ae628bd39a47aefa680744bf41a32bb31afe22d3da16e4f293039bcad7274bbaf4b65891c655e0cf8998b98cdd7dc14ff8b4bded053f80e6a0f4ebefdcd57024100c3dc8bcd2f7a5e6569a20ebd59db73b6675a610a51821fd067246e811cb510b61caf0bc66da4831ae77104fd11208a727efaa99a3dbcc20b180f0e477e364e1f024100c0d169dec2a2782d2d23c7645bda848acf8fa7820bdeb1db44f6792e3747f4ad16b030cbccd76f26039765399c8823b58515c5f6af812107538cd9c9971a2637024031bf64eafaf233e3c24edd3b8f054480c2039cbe4831aaeefe23acc5b28af2f1ae9b6f7c39011e23c94155a9099ea04bb0a0ee4f34fb2ab632a830524c6672b5024063eced81583cc2521045f0686994f9bb3b5108e562a58659aa332cb4a5527bebf839438ea41eeb288c79fbd3ab54eb72af24efcb06b89ccf8a582566d45cdf8d");

            PeerInfo.Builder builder = new PeerInfo.Builder(PeerInfo.Builder.flagsHasSecret);
            PeerConstInfo constInfo = new PeerConstInfo();
            constInfo.continent = 0;
            constInfo.country = 0;
            constInfo.city = 0;
            constInfo.carrier = 0;
            constInfo.inner = 0;
            constInfo.deviceId = "local";
            constInfo.publicKey = publicKey;
            localPeer = builder.setConstInfo(constInfo)
                    .addEndpoint("v4tcp127.0.0.1:10000")
                    .setSecret(secret)
                    .finish();
        } catch (Exception e) {
            Log.e("local peer", e.toString());
        }

        PeerInfo remotePeer = null;
        PeerConstInfo remoteConstInfo = null;
        try {
            byte[] publicKey = hexToByte("30819f300d06092a864886f70d010101050003818d00308189028181008659bc3e9954e47a331126aadaf580e8e9af9e89f4934f74926526448c2030925ee33416644086c73dc6e0bc3075a6783c3932872759d0524c2f17999c57c11ce67acc469c479c36188fd6c76a2b5a4c680e981f4e834331d07113234f3bbdc00570510cf58b0c5ebbab860ae8faa39af5a3cb8ea1d0760e21b3e1dbfe0355e70203010001");
            byte[] secret = hexToByte("3082025d020100028181008659bc3e9954e47a331126aadaf580e8e9af9e89f4934f74926526448c2030925ee33416644086c73dc6e0bc3075a6783c3932872759d0524c2f17999c57c11ce67acc469c479c36188fd6c76a2b5a4c680e981f4e834331d07113234f3bbdc00570510cf58b0c5ebbab860ae8faa39af5a3cb8ea1d0760e21b3e1dbfe0355e7020301000102818072b94fe8f89028663d13493c9eab03c06044aa11cdbab91fa71f1eb56c1ed4bb38b1b539e5b3c023851a3db015857178bc9c6f0c404b0e7c2838126406c05cd2ede55a21425421bc1aa72fb68f82db9e7533d50dcd040f5f5f401a3a0ce6be1148db75d024cc02ec55cbbf78ca345302fe158ab95f45ecc491575da0d8579a31024100cb24aff0f3e47f4d6e379b1b77883e5fac5b7dac7265dee6a9a6b849de92b791e029567c70febd8ae7eebe95aeb9db3fd8e94d9367ba6be61a79c2236cb720b9024100a94ec84461edff5edbb97acbb186ee0bceea6266e233105181d5653a651d833ee119cebd0f9c556060de353f3213f73bc22cbe96333dc971a4f580b19c269b9f024100ac6435eb050ea3f9d1cede92309e2e5082b421b276627d06c271f972b6af4b993fe1d4c34620e839391a22226464d4eb19e8e32c749a7f7686814d7f4283260102407a3d0440c307c789e050414551ce5e8e2dfd71c0606e87c8a159c5f56c4deb9579865d8a88fbd1747d5bd1cbe7c71c888bc02c765b56afdb9a431a80a1820a7702410099a61250e658d2fad81601de5ed00e1648f5001b9e18c21ff28f0b1f2a003de6457c292274e02abdad55d0f6e48d6726b7d6cb7f5ee958457397e3dbca1f39fe");

            PeerInfo.Builder builder = new PeerInfo.Builder(PeerInfo.Builder.flagsHasSecret);
            PeerConstInfo constInfo = new PeerConstInfo();
            constInfo.continent = 0;
            constInfo.country = 0;
            constInfo.city = 0;
            constInfo.carrier = 0;
            constInfo.inner = 0;
            constInfo.deviceId = "local";
            constInfo.publicKey = publicKey;
            remoteConstInfo = constInfo;
            remotePeer = builder.setConstInfo(constInfo)
                    .addEndpoint("v4tcp127.0.0.1:10001")
                    .setSecret(secret)
                    .finish();

        } catch (Exception e) {
            Log.e("remote peer", e.toString());
        }



        Stack localStack = new Stack();
        {
            PeerInfo[] intialPeers = {remotePeer};
            localStack.open(
                    localPeer,
                    intialPeers,
                    new Stack.EventListener() {
                        @Override
                        public void onOpen() {
                            Log.i("local peer", "opened");
                        }
                        @Override
                        public void onError(int error) {
                            Log.e("local peer", "error " + error);
                        }
                    });
        }

        Stack remoteStack = new Stack();
        {
            PeerInfo[] intialPeers = {};
            remoteStack.open(
                    remotePeer,
                    intialPeers,
                    new Stack.EventListener() {
                        @Override
                        public void onOpen() {
                            Log.i("remote peer", "opened");
                        }
                        @Override
                        public void onError(int error) {
                            Log.e("remote peer", "error " + error);
                        }
                    });
        }

        Connection.listen(remoteStack, (short)0);
        Connection.accept(
                remoteStack,
                (short)0,
                new Connection.PreAcceptListener() {
                    @Override
                    public void onError(int error) {
                        Log.e("remote peer", "error " + error);
                    }
                    @Override
                    public void onConnection(Connection conn, byte[] question) {
                        Log.i("remote connection", "pre accept with question:" + new String(question));
                        String answer = "first answer";
                        final Connection fconn = conn;
                        Connection.AcceptListener acceptListener = new Connection.AcceptListener() {
                            @Override
                            public void onEstablish() {
                                Log.i("remote connection", "established");
                                byte[] buffer = new byte[100];
                                Connection.RecvListener recvListener = new Connection.RecvListener(buffer) {
                                    public void onRecv(long recv) {
                                        String data = null;
                                        try {
                                            data = new String(this.getBuffer(), "utf-8");
                                        } catch (Exception _) {

                                        }

                                        Log.i("remote connection", "onRecv: " + data);
                                    }
                                };
                                fconn.recv(recvListener);
                            }
                            @Override
                            public void onError(int error) {
                                Log.i("remote connection", "onError: " + error);
                            }
                        };
                        conn.confirm(answer.getBytes(), acceptListener);
                    }
                });


        Connection.ConnectListener connectListener = new Connection.ConnectListener() {
            @Override
            public void onAnswer(byte[] answer) {
                Log.i("local connection", "onAnswer: " + new String(answer));
            }
            @Override
            public void onEstablish() {
                Log.i("local connection", "established");
                String data = "first data";
                Connection.SendListener sendListener = new Connection.SendListener(data.getBytes()) {
                    @Override
                    public void onSent(int error, long sent) {
                        Log.i("local connection", "onSent: " + sent);
                    }
                };
                this.m_connection.send(sendListener);
            }
            @Override
            public void onError(int error) {
                Log.e("local connection", "onError: " + error);
            }
        };

        String question = "first question";
        byte[][] snList = {};
        Connection.connectRemote(
                localStack,
                remoteConstInfo,
                (short)0,
                snList,
                question.getBytes(),
                connectListener);
    }


    public static byte[] hexToByte(String hex){
        int m = 0, n = 0;
        int byteLen = hex.length() / 2; // 每两个字符描述一个字节
        byte[] ret = new byte[byteLen];
        for (int i = 0; i < byteLen; i++) {
            m = i * 2 + 1;
            n = m + 1;
            int intVal = Integer.decode("0x" + hex.substring(i * 2, m) + hex.substring(m, n));
            ret[i] = Byte.valueOf((byte)intVal);
        }
        return ret;
    }
}
