class Connection {
    constructor() {
        this.m_c_connection;
        this.m_caches = [Buffer.allocUnsafe(4096), Buffer.allocUnsafe(4096)];
        this.m_filledCaches = [];
        this.m_recvBegin = false;
        this.m_recvs = {
            buffer: null,
            callback: null,
            size: null,
            pos: null,
        };

        this.m_lock;
    }

    // port层不了解connection状态，在js层第一次投递recv时投递双buffer
    recv(buffer, callback) {
        function recvCallback(buffer, size, userdata) {
            let offset = 0;
            let completeRecvs = [];
            this.m_lock.lock();
            while (this.m_recvs.length > 0) {
                recvJs = this.m_recvs.shift();
                buffer.copy(recvJs.buffer, 0, offset, size);
                offset += recvJs.buffer.length;
                completeRecvs.push(recvJs);
                if (offset === size) {
                    break;
                }
            }

            // 多余的buffer，存起来
            if (offset < size) {
                // 合并cache可能提高并发，但可能增加copy
                if (this.m_filledCaches.length > 0) {
                    let lastCache = this.m_filledCaches[this.m_filledCaches.length - 1];
                    if (lastCache.size - lastCache.pos > size - offset) {
                        buffer.copy(lastCache, lastCache.pos, offset, size);
                    } else {
                        this.m_filledCaches.push({buffer, size, pos});
                    }
                } else {
                    this.m_filledCaches.push({buffer, size, pos});    
                }
           }
            this.m_lock.unlock();

            if (offset === size) {
                this.m_c_connection.recv(buffer, recvCallback, userdata);
            }

            for (let recvJs of completeRecvs) {
                completeRecvs.callback.threadSafe(recvJs.buffer, recvJs.pos);
            }
        }

        if (!this.m_recvBegin) {
            // 第一次投递，直接投递port层cache到C层，并且不存在线程同步问题
            this.m_recvBegin = true;
            this.m_recvs.push({buffer, callback});
            
            for (let cache of this.m_caches) {
                this.m_c_connection.recv(cache, recvCallback, {connection: this});
            }
        } else {
            // 已经有数据缓存，直接回调
            let clearedCaches = [];
            this.m_lock.lock();
            for (let filledCache of this.m_filledCaches) {
                filledCache.copy(buffer);
                // <TODO>处理buffer重组
            }
            clearedCaches = this.m_filledCaches.splice(0, partSize); // <TODO> this.m_filledCaches可能没有清完，需要处理
            this.m_filledCaches.clear();
            if (clearedCaches.length === 0) {
                this.m_recvs.push({buffer, callback});
            }
            this.m_lock.unlock();

            for (let cache of clearedCaches) {
                this.m_c_connection.recv(cache, recvCallback, {connection: this});
            }

            // 已经用port层缓存填充，立即返回
            if (clearedCaches.length > 0) {
                callback(buffer);
            }
        }

    }
}