#include "header.h"

/*
main.cで行う機能

設定ファイルの読み込み
コマンドライン引数の読み取り
クライアントモード、サーバーモードへの遷移
エラー表示
終了
*/
#include "server.c"
#include "client.c"

int main(int argc, char *argv[]){
    commandline_t mode = { 0 , 0 } ;
    config_t config;                                //コンフィグ読み込み構造体
    config_load(&config);
    
    error_message( mode_check( argv, &mode , argc) );
    if ( mode.server != 0 ){
        error_message(server_main());               //サーバーモードへ
    } else if ( mode.client != 0 ){
        error_message(client_main(&config));               //クライアントモードへ
    }
    return 0;
}

//コマンドラインオプションを読み込むーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー
int mode_check(char *argv[], commandline_t *commandline,int argc){
//	ループ毎に1つずつコマンドラインオプションを取得する：arg_check(argv[i])
    if ( argc > 2 ){                                //引数は最大2個まで
        return ERROR_ARG_TOO_MANY;
    }
    for(int i = 1; i < argc; i++){                  //1つ目のパラメータは実行ファイルなので弾く
        switch ( arg_check(argv[i]) ) { 
          case 's':
                commandline->server = i;
            break;

          case 'c':
                commandline->client = i;
            break;

          default:
            return ERROR_ARG_UNKNOWN;
            break;
        }
    }
    return NO_ERROR;
}

int arg_check(char* arg){
    if ( (arg[0] == '-') && (arg[1] > 32) ){        //制御文字,空白文字指定は無視
        return arg[1];
    }                                               //オプションの2文字目を返す
    return 0;
}
//コンフィグを読み込む
int config_load(config_t *cfg){
    char configfile[256];
    
    memset(cfg,'\0',sizeof(config_t));
    memset(configfile,'\0', 256);

    FILE *fp;
    fp = fopen(CONFIG_FILE,"r");                    //readモードで開く

    if ( fp == NULL ){                              //コンフィグファイルがない
        fclose(fp);                                 //ファイルを閉じる
        return CONFIG_NOTFOUND;
    } else {
        while( fgets(configfile , 255 , fp) != NULL ){  //1行ずつ読み出す(最大255バイト)
            config_param(configfile, "host", cfg);
        }
        fclose(fp);
    }
    return NO_ERROR;
}
//configのパラメータを切り出し、cfg->symbolnameに代入する
int config_param(char *configfile, char *symbolname, config_t *cfg){
    char param[256];
    int i = 0;
    int j = 0;
        
    memset(param,'\0',256);
        
    if ( !strncmp(configfile , symbolname, strlen(symbolname)) ){  //文頭がsymbolname
        while ( configfile[i++] != '=') {                  //=が来るまで送る 1バイト目が=でないことは前条件式で保障されている
            if ( i > strlen(configfile)){
                break;                                     //=がなければループ抜け
            }
        }
        while ( configfile[i] == ' ') {
            i++;                                           //スベースが入っていればその分文字を進める
        }
        while ( !( (configfile[i] == '\n') || (configfile[i] == '\0')) ){
            param[j] = configfile[i];                      //\0でも\nでもない場合にコピー
            i++;
            j++;
        }
        param[j] = '\0';
        strncpy(cfg->host,param,119);                      //コンフィグ構造体にコピーする

        return 1;
    }
    return 0;
}

//エラーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー
void error_message(int err){
    switch (err){				//エラーが来ていなければ素通り
      case NO_ERROR:
        break;
        
      case ERROR_ARG_UNKNOWN:
        printf("モード指定が不正です。\n");
        exit(0);
        
      case ERROR_ARG_TOO_MANY:
        printf("モード指定が多すぎます。\n");
        exit(0);
        
      case ERROR_ARG_SEVERAL_MODES:
        printf("モードが複数指定されています。\n");
        exit(0);
        
      case ERROR_SOCKET:
        printf("ソケットの作成に失敗しました。\n");
        exit(0);
        
      case ERROR_SOCKET_OPTION:
        printf("ソケットオプションの設定に失敗しました。\n");
        exit(0);
        
      case ERROR_SOCKET_BIND:
        printf("ソケットのバインドに失敗しました。\n");
        exit(0);
        
      case ERROR_SOCKET_ACCEPT:
        printf("ソケット通信の受付に失敗しました。\n");
        exit(0);
        
      case ERROR_SOCKET_CLOSE:
        printf("ソケットのクローズに失敗しました。\n");
        exit(0);
        
      case ERROR_HOST_UNKNOWN:
        printf("ホスト名を解決できませんでした。\n");
        exit(0);
        
      case ERROR_CONNECT:
        printf("サーバに接続できませんでした。");
        break;
        
      case ERROR_FILENOTFOUND:
        printf("ファイルがありません。\n");
        break;
        
      case ERROR_TOO_MANY_CONNECT:
        printf("503 ERROR\n");
        break;
        
      default:
        printf("不正なエラーです。\nエラーコード:%d\n" , err);
        exit(0);
    }
}