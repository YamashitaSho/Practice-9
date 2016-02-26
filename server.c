#include "header.h"

/*
server.cでやること

子スレッドを作成し、サーバプログラムとしてポート8888で待機する。
接続が確立したら同時に10個まで子スレッドを作り、接続待機状態を維持する。
clientからファイル名が送られてくるので、文字列ファイルを読み込んで内容を返信する。
*/

//接続状態を監視しつつ待機し、接続があれば新しい子スレッドを作る。
int server_main(){
     int i;
     int err;
     int sock_optival = 1;
     int port = DEFAULT_PORT;
     int listening_socket;
     int state_check;                               //スレッドの総合的な接続状況を判断するための変数
     pthread_t thread_id[CONNECT_MAX+1];            //スレッド識別変数(最後の一つはエラー用)
     struct addrinfo hints;                         //getaddrinfoに渡すデータ
     struct addrinfo *result;                       //getaddrinfoから受け取るデータ
     struct addrinfo *rp;
     struct sockaddr_in peer_sin;
     socklen_t len = sizeof(struct sockaddr_in);
     
     
     for (i=0 ; i < CONNECT_MAX+1 ; i++){
          thread[i].state = -1;                     //初期化
     }
     memset(&hints,0,sizeof(hints));
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;
     hints.ai_flags = AI_PASSIVE;
     hints.ai_protocol = 0;
     hints.ai_canonname = NULL;
     hints.ai_addr = NULL;
     hints.ai_next = NULL;
     
     err = getaddrinfo(NULL,"8888",&hints,&result);
     if (err != 0){
          printf("getaddrinfo...\n");
          exit(0);
     }
     rp = result;                                   //getaddrinfoで受け取ったアドレス構造体の配列
     for (rp = result; rp != NULL; rp = rp->ai_next) { //構造体ごとにバインドを試す
          listening_socket = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
          if (listening_socket == -1){              //ソケットが作れなかったらスルー
               printf("a\n");
               continue;
          }
          setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &sock_optival, sizeof(sock_optival) );
          if (bind(listening_socket, rp->ai_addr, rp->ai_addrlen) == 0){
            break;                                  //バインド成功
          }
          close(listening_socket);                  //バインドに失敗したのでソケットを閉じる
     }
     if (rp == NULL) {                              //有効なアドレス構造体がなかった
          error_message(ERROR_SOCKET_BIND);
     }
     freeaddrinfo(result);
     //ポートを見張るようにOSに命令する
     err = listen ( listening_socket, SOMAXCONN) ;
     if ( err == -1 ){
          error_message(ERROR_SOCKET_LISTEN);
     }
     printf("使用ポート:%d\n", port);
     while(1){
          //thread.state(1:接続中 0:接続待機 -1:ヒマ)
          state_check = 0;
          for (i=0 ; i < CONNECT_MAX ; i++){
               if (thread[i].state == 1 ){
                    state_check ++;
               }
               printf("%d.",thread[i].state);   //デバッグ用
          }
          
          printf("接続数:(%d/%d)\n",state_check,CONNECT_MAX);
          for (i = 0; i < CONNECT_MAX+1 ; i++){
               if (thread[i].state == -1){
                    //ステータスがヒマだった通信用ソケットを使用して接続待機
                    thread[i].socket = accept( listening_socket , (struct sockaddr *)&peer_sin , &len );
                    if ( thread[i].socket == -1){
                         error_message(ERROR_SOCKET_ACCEPT);
                    }
                    if ( i < CONNECT_MAX ){                                        //CONNECT_MAXを超えていない
                         connection_accepted(thread[i].socket);                    //メッセージを送る:000(エラーなし)
                         printf("接続しました:%s\n",inet_ntoa(peer_sin.sin_addr));   //クライアントのIPアドレスを表示
                         thread[i].state = 0;                                      //スレッドのステータスを接続待ちに変える
                         
                         pthread_create(&thread_id[i], NULL, (void *)connect_thread, &thread[i]); //スレッドを作る
                         pthread_detach(thread_id[i]);                                            //スレッドの終了は待たない
                         break;
                    } else {                                                       //CONNECT_MAXを超えている
                         too_many_connection(thread[i].socket);                    //メッセージを送る:503(アクセス過多)
                         printf("接続過多により接続を拒否します:%s\n",inet_ntoa(peer_sin.sin_addr));
                         
                         err = close(thread[i].socket);                            //通信用ソケットを破棄
                         
                         if (err == -1){
                              error_message(ERROR_SOCKET_CLOSE);
                         }
                    }
               }//if(thread[i].state == -1)
          }//for
     }//while
     
     err = close(listening_socket);
     if (err == -1){
          error_message(ERROR_SOCKET_CLOSE);
     }
     return 0;
}

//子スレッドでの処理
void connect_thread(threadinfo_t *thread){
     int err;
     
     while (1){
          thread->state = 1;                                //スレッドのステータスを接続中に変える
          server_receive_transmission(thread->socket);      //データのやりとり

          err = close(thread->socket);
          if (err == -1){
               error_message(ERROR_SOCKET_CLOSE);
          }
          thread->state = -1;                               //スレッドのステータスを解放済みに変える
          printf("接続が切れました。\n");
          break;
     }
}

int server_receive_transmission(int socketid){
     char filedata[4391];
     char filename[256];
     
     memset(filedata,'\0',4391);
     memset(filename,'\0',256);
     
     error_message( receive_filename(socketid , filename) );
     error_message( transmission_filedata(socketid , filename, filedata) );
     
     return NO_ERROR;
}

int receive_filename(int socketid, char *filename){
     char buf[256];
     char bufc = '\0';
     
     memset(buf,'\0',256);
     
     while (1){
          read(socketid , &bufc , 1);
          if ( bufc == '\0' ){
               break;
          } else {
               strncat(buf , &bufc , 1);
          }
     }
     printf("受信しました。>>%s\n",buf);
     strcpy(filename,"root/");
     strncat(filename,buf,250);
     return 0;
}

int transmission_filedata(int socketid, char *filename, char *filedata){
     char buf[256];                                 //ファイル読込用バッファ
     int buf_len = 0;
     memset(buf,'\0',256);
     
     FILE *fp;
     fp = fopen(filename,"r");
     if ( fp == NULL ){
          printf("FILE NOT FOUND\n");
          strncpy(filedata , "F00" , 3);            //ファイルがなかった場合のヘッダ -> F00
     } else {
          strncpy(filedata , "T00" , 3);            //ファイルがあった場合のヘッダ   -> T00
          while( fgets(buf,255,fp) != NULL ){
               strncat(filedata , buf , 255);       //fpが空になるまでfiledataに詰め込む
               if (strlen(filedata) > 4096){
                    break;
               }
          }
     }
     
     while(1){
          printf("buf_len:%d\n",buf_len);
          if ( filedata[buf_len] == '\0' ){
               break;
          }
          buf_len += write(socketid, (filedata+buf_len) , 255);
     }
     printf("送信しました。\n>>\n%s\n",filedata);
     
     return NO_ERROR;
}
void connection_accepted(int socketid){
     char tmc[4] = "000";
     int buf_len = 0;
     
     while(1){
          if ( tmc[buf_len] == '\0'){
               break;
          }
          buf_len += write(socketid, (tmc+buf_len) , 1);
     }
}

void too_many_connection(int socketid){
     char tmc[4] = "503";
     int buf_len = 0;
     
     while(1){
          if ( tmc[buf_len] == '\0'){
               break;
          }
          buf_len += write(socketid, (tmc+buf_len) , 1);
     }
     printf("503 ERROR\n");
}