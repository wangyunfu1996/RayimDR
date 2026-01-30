#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import threading
import sys
import time
from datetime import datetime

class TcpEchoServer:
    def __init__(self, host='0.0.0.0', port=4242, heartbeat_interval=5):
        self.host = host
        self.port = port
        self.server_socket = None
        self.running = False
        self.heartbeat_interval = heartbeat_interval
        self.clients = []
        self.clients_lock = threading.Lock()
        
    def handle_client(self, client_socket, client_address):
        """处理客户端连接"""
        print(f"客户端已连接: {client_address}")
        
        # 添加到客户端列表
        with self.clients_lock:
            self.clients.append({
                'socket': client_socket,
                'address': client_address
            })
        
        try:
            while self.running:
                # 设置超时，避免阻塞过久
                client_socket.settimeout(1.0)
                try:
                    data = client_socket.recv(1024)
                    if not data:
                        print(f"客户端 {client_address} 断开连接")
                        break
                    
                    print(f"收到数据 [{len(data)} bytes] 从 {client_address}: {data.decode('utf-8', errors='ignore').strip()}")
                    
                    # 回显数据
                    client_socket.sendall(data)
                    print(f"已回显数据到 {client_address}")
                    
                except socket.timeout:
                    # 超时继续循环
                    continue
                
        except Exception as e:
            print(f"处理客户端 {client_address} 时出错: {e}")
        finally:
            # 从客户端列表移除
            with self.clients_lock:
                self.clients = [c for c in self.clients if c['socket'] != client_socket]
            
            client_socket.close()
            print(f"客户端 {client_address} 连接已关闭")
    
    def heartbeat_sender(self):
        """周期性向所有客户端发送心跳消息"""
        counter = 0
        while self.running:
            time.sleep(self.heartbeat_interval)
            if not self.running:
                break
                
            counter += 1
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            message = f"[SERVER HEARTBEAT #{counter}] {timestamp}\n"
            
            with self.clients_lock:
                disconnected_clients = []
                for client in self.clients:
                    try:
                        client['socket'].sendall(message.encode('utf-8'))
                        print(f"发送心跳消息到 {client['address']}: {message.strip()}")
                    except Exception as e:
                        print(f"发送心跳到 {client['address']} 失败: {e}")
                        disconnected_clients.append(client)
                
                # 清理断开的客户端
                for client in disconnected_clients:
                    self.clients.remove(client)
    
    def start(self):
        """启动服务器"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)
            # 设置超时，使accept()不会无限期阻塞
            self.server_socket.settimeout(1.0)
            self.running = True
            
            print(f"TCP Echo Server 启动成功")
            print(f"监听地址: {self.host}:{self.port}")
            print(f"心跳间隔: {self.heartbeat_interval}秒")
            print("等待客户端连接...\n")
            
            # 启动心跳发送线程
            heartbeat_thread = threading.Thread(target=self.heartbeat_sender)
            heartbeat_thread.daemon = True
            heartbeat_thread.start()
            
            while self.running:
                try:
                    client_socket, client_address = self.server_socket.accept()
                    # 为每个客户端创建新线程
                    client_thread = threading.Thread(
                        target=self.handle_client, 
                        args=(client_socket, client_address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    
                except socket.timeout:
                    # 超时继续循环，这样可以检查 self.running 状态
                    continue
                except KeyboardInterrupt:
                    print("\n收到中断信号，正在关闭服务器...")
                    break
                except Exception as e:
                    if self.running:
                        print(f"接受连接时出错: {e}")
                    
        except Exception as e:
            print(f"启动服务器失败: {e}")
        finally:
            self.stop()
    
    def stop(self):
        """停止服务器"""
        self.running = False
        if self.server_socket:
            self.server_socket.close()
        print("服务器已停止")

def main():
    host = '0.0.0.0'
    port = 4242
    heartbeat_interval = 1
    
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    if len(sys.argv) > 2:
        host = sys.argv[2]
    if len(sys.argv) > 3:
        heartbeat_interval = int(sys.argv[3])
    
    server = TcpEchoServer(host, port, heartbeat_interval)
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\n正在退出...")
    except Exception as e:
        print(f"服务器错误: {e}")

if __name__ == '__main__':
    main()
