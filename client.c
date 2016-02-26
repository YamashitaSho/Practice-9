#include "header.h"

/*
client.cでやること
メニューの表示
1.ファイル取得
サーバプログラムに接続し、ファイル名を送信する。
サーバプログラムから返信されたファイルの内容を受け取る。
2.設定
設定ファイルを読み込む。なければ作成する。
9.終了
*/

int client_main(config_t *cfg){
    int mode;
    
    mode = get_menu();                              //モード選択はループの最後に行う
    while(1){
        if ( mode == CLIENT_GET ){                  //ファイル取得
            if ( cfg->host[0] == '\0' ){
                printf("接続先が登録されていません。\nEnterキーでメニューに戻ります。\n");
                fflush(stdin);
                getc(stdin);
                mode = get_menu();
                continue;
            }
            if ( client_receive_transmission(cfg) == ERROR_CONNECT ){
                printf("サーバに接続できませんでした。\n接続先の受信プログラムが起動していない可能性があります。\nEnterキーでもう一度接続を試みます。\n");
                fflush(stdin);
                getc(stdin);
                continue;
            }
        } else if ( mode == CLIENT_CONFIG ){
            if ( cfg->host[0] == '\0' ){            //接続先がないので入力させる
                printf("接続先を入力し、Enterキーを押してください。\n>>");
                new_config(cfg);
                config_load(cfg);
            } else {                                //接続先を表示し、入力を求める。
                printf("現在設定されている接続先は%sです。\n上書きしない場合は、Enterキーのみを入力してください。\n上書きする場合は接続先を入力し、Enterキーを入力してください。\n>>",cfg->host);
                new_config(cfg);
                config_load(cfg);
            }
        } else if ( mode == CLIENT_EXIT ){          //終了
            exit(0);
        }
        mode = get_menu();
    }
    return NO_ERROR;
}

char get_menu(){
    char input[50];
    char mode;
    
    printf("\n1.ファイル取得\n2.設定\n9.終了\n\nメニュー番号を入力してください:");
    fflush(stdin);
    fgets(input,50,stdin);
    mode = input[0];
    
    return mode;
}

void input_a_line(char *inputline){
    char input[256];
    char *newline;                                  //改行検出用
    
    printf("\n読み込むファイル名を入力してください。>>");
    fflush(stdin);
    fgets(input , 255 , stdin);
    
    if (input[0] != '\n') {
        newline = memchr(input , '\n', 255);        //fileの終端にある改行コードを検出する
        *newline = '\0';                            //'\0'に置き換える
    }
    strncpy(inputline , input , 255);
}
//コンフィグファイルを上書きする(新規作成も含む)
int new_config(config_t *cfg){
    char input[256];
    char host[261];
    char *newline;
    FILE *fp;
    
    memset(input,'\0',256);
    memset(host,'\0',256);
    
//  printf("接続するホストを入力してください。\n>>");
//  この関数を呼び出す側でメッセージを出力しておく。
    fflush(stdin);
    fgets(input , 255 , stdin);
    
    if (input[0] == '\n') {
        printf("ファイルは変更されませんでした。");
        return 1;
    } else {
        newline = memchr(input , '\n', 255);        //fileの終端にある改行コードを検出する
        *newline = '\0';                            //'\0'に置き換える
        fp = fopen(CONFIG_FILE,"w");
        sprintf(host,"host=%s",input);
        fputs(host,fp);
        printf("接続先が%sに変更されました。",input);
        fclose(fp);
    }
    return 0;
}

//受送信
int client_receive_transmission(config_t *cfg){
    int connecting_socket = 0;
    struct sockaddr_in client_addr;                 //ネットワーク設定
    struct in_addr servhost;                        //サーバーのIP格納
    int port = DEFAULT_PORT;                        //ポート番号
    int ret;                                        //返り値の一時保存
    char connect_error[4];                        //接続時のエラー受信用
    char receive_data[4391];                        //受け取るデータ

    memset(receive_data,'\0',4391);
    memset(connect_error,'\0',4);
    
    //ソケットを作成
    if ( (connecting_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error_message(ERROR_SOCKET);
    }
    
    printf("接続します:%s\n", cfg->host );
    
    //ホスト名の解決
    if ( address_resolution(cfg, &servhost) == 0){
        return ERROR_HOST_UNKNOWN;
    }
    //アドレスファミリー・ポート番号・IPアドレス設定       設定ファイルを適用する部分
    memset(&client_addr, '\0' ,sizeof(client_addr));
    client_addr.sin_family = AF_INET;
   // client_addr.sin_port = htons(port);
        client_addr.sin_port = htons(port);
    client_addr.sin_addr = servhost;
    
    ret = connect( connecting_socket, (struct sockaddr *)&client_addr, sizeof(client_addr) );
    if ( ret < 0 ){
        return ERROR_CONNECT;
    }
    
    receive_header(connecting_socket,connect_error);
    if ( strncmp(connect_error,"503",3) == 0 ){
        close(connecting_socket); 
        return ERROR_TOO_MANY_CONNECT;
    }
    
    printf("サーバーに接続しました。\n");
    transmission_filename(connecting_socket);
    receive_filedata(connecting_socket, receive_data);
    
    
    close( connecting_socket );

    return NO_ERROR;

}
//ホスト名を解決
int address_resolution(config_t *cfg, struct in_addr *servhost){
    struct addrinfo hint;                           //getaddrinfoを使うためにaddrinfo型の構造体を宣言
    struct addrinfo *result;                        //getaddrinfoの結果を受け取る構造体
    
    memset( &hint , 0 , sizeof(struct addrinfo) );
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = 0;
    hint.ai_protocol = 0;
    
    if ( getaddrinfo(cfg->host,NULL,&hint,&result) != 0 ) {
        return 0;
    }
    servhost->s_addr=((struct sockaddr_in *)(result->ai_addr))->sin_addr.s_addr;
//  printf("ip address:%s\n",inet_ntoa(*servhost));
    
    freeaddrinfo(result);
    return 1;
}
//ヘッダー受信
int receive_header(int socketid ,char *header){
    char bufc = '\0';
    
    memset(header,'\0',4);
    int i = 0;
    while(1){
        if (i == 3) {
            break;
        }
        if ( read(socketid,&bufc,1) == 1 ){
            header[i] = bufc;
            i++;
        }
    }
    printf("HEADER:%s\n",header);
    return NO_ERROR;
}
    
//ファイル名送信
int transmission_filename(int socketid){
    int buf_len = 0;
    char filename[256];
    
    while(1){
        memset(filename,'\0',256);
        input_a_line(filename);
        if ( filename[0] == '\n' ){
            continue;
        }
        break;
    }
    while(1){
        if ( filename[buf_len] == '\0' ){
            break;
        }
        buf_len += write( socketid , (filename+buf_len) , 255 );
    }
    printf("ファイル名:%sを送信しました。\n",filename);
    return NO_ERROR;
}
//ファイル内容受信
int receive_filedata(int socketid, char *receive_data){
    int i;
    char bufc[256];
    char header[4];
    memset(header,'\0',4);
    for(i=0;i<3;i++){
        memset(bufc,'\0',256);
        read(socketid,bufc,1);
        header[i] = bufc[0];
    }
    printf("HEADER:%s\n",header);
    if ( header[0] == 'T'){
        while (1){
            memset(bufc,'\0',256);
            read ( socketid , bufc , 1);
            if ( bufc[0] == '\0' ){
                break;
            } else {
                strncat(receive_data , bufc , 1);
            }
        }
        printf("%s\n%luバイト受信しました。\n",receive_data,strlen(receive_data));
    } else if ( header[0] == 'F' ){
        printf("ファイルが見つかりませんでした。\n");
    } else {
        printf("HEADER NOT FOUND");
    }
    
    return NO_ERROR;
}

