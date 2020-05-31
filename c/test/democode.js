//framework 

var framework = {};

framework.onevent = function() {

}

framework.createTcpClientSocket = function (bindAddr) {
    var s = create_tcp_socket();
    if(s.bind(bindAddr)) {
        s.onevent = framework.onevent;
    }
    
    return s;
}

framework.TcpClientSocketConnect = function(remoteAddr,remotePort) {
    
}
