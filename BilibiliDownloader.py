'''From:https://github.com/johnlimit/Bilibili-Download-Assistant'''
import os
import time
import platform
try:
    import requests
except:
    print('检测到缺少requests库,正在安装...')
    os.system('pip3 install requests')
    import requests
import re
import json
try:
    from colorama import Fore,Style
except:
    print('检测到缺少colorama库,正在安装...')
    os.system('pip3 install colorama')
    from colorama import Fore,Style
try:
    from tqdm import tqdm
except:
    print('检测到缺少tqdm库,正在安装...')
    os.system('pip3 install tqdm')
    from tqdm import tqdm
try:
    from bs4 import BeautifulSoup
except:
    print('检测到缺少bs4库,正在安装...')
    os.system('pip3 install bs4')
    from bs4 import BeautifulSoup

class BilibiliDownloader:
    def __init__(self,bv_number,dirname='download'):
        self.bv_number = bv_number
        self.dir_name = dirname
        self.audio_url = ''

    '''获取合集内音频数量'''
    def get_collection_num(self):
        url = 'https://www.bilibili.com/video/' + self.bv_number + '?p=1'
        html = self.get_html(url)
        soup = BeautifulSoup(html.text,'html.parser')
        try:
            collection_num = (soup.find('span',attrs={'class':'cur-page'}).contents[0])[3:-1]
        except:
            return -1
        return int(collection_num)

    '''下载WEB页面数据'''
    def get_html(self,url,is_stream=False):
        headers = {
            'referer':'https://www.bilibili.com',
            'user-agent': 'Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36'
        }
        if is_stream == True:
            html = requests.get(url,headers=headers,stream=True)
        else:
            html = requests.get(url,headers=headers)
        return html

    '''从合集列表提取URL'''
    def parser(self,html):
        json_data = re.findall('<script>window\.__playinfo__=(.*?)</script>', str(html))[0]
        json_data = json.loads(json_data)
        try:
            self.audio_url = json_data['data']['dash']['audio'][0]['backupUrl'][0]
        except Exception as e:
            print('无法抓取BackupURL，错误类型为')
            print(type(e))
            print('转为抓取base_url.')
            try:
                self.audio_url = json_data['data']['dash']['audio'][0]['base_url']
            except Exception as eg:
                print('无法抓取base_url，错误类型为')
                print(type(eg))
                exit()

    '''从合集列表提取名字'''
    def get_fname(self,html,num):
        json_data = re.findall('<script>window\.__INITIAL_STATE__=(.*?)</script>', str(html))[0]
        index = json_data.rfind(';(function')
        json_data = json.loads(json_data[:index])
        filename = json_data['videoData']['pages'][num-1]['part']
        return filename.strip('《').strip('》')
        #return json_data['videoData']['pages'][num-1]['part']

    '''下载文件数据并保存'''
    def download_data(self,collection_name='1433juh8j4rk'):
        resp = self.get_html(self.audio_url,is_stream=True)
        total = int(resp.headers.get('content-length',0))
        if collection_name == '1433juh8j4rk':
            collection_name = self.bv_number
        with open(self.dir_name + "\\" + collection_name + '.mp3',mode='wb') as f,tqdm(
            desc=collection_name+'.mp3',
            total=total,
            unit='iB',
            unit_scale=True,
            unit_divisor=1024
        )as bar:
            for data in resp.iter_content(chunk_size=1024):
                size = f.write(data)
                bar.update(size)

def pause():
    if platform.system() == 'Windows':
        os.system('pause')
    else:
        input('按回车键继续...')

def main():
    save_dir_name = 'download'
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    while True:
        print(Style.RESET_ALL)
        print('请选择操作:\n\t1:下载单个音频\n\t2:下载音频合集\n\t3:退出程序')
        choice = input('请输入:')
        if choice == '1':
            is_download_collection = False
            bvid = input('请输入要下载音频的BV号(以BV开头):')
        elif choice == '2':
            is_download_collection = True
            bvid = input('请输入要下载合集的BV号(以BV开头):')
        elif choice == '3':
            break
        else:
            continue
        if bvid.upper().find('BV') == -1:
            print('输入错误.请注意:输入以BV开头的BV号!')
            continue
        if not os.path.exists(save_dir_name):
            print("创建目录:"+save_dir_name)
            os.mkdir(save_dir_name)
        elif is_download_collection == False and os.path.exists(save_dir_name+"\\"+bvid+'.mp3'):
            print('您正在下载的音频文件'+bvid+'.mp3已存在!\n继续下载将导致原文件被覆盖!')
            choice3 = input('请选择:(1:继续下载 0:取消下载)')
            if choice3 != '1':
                continue
            else:
                os.remove(save_dir_name+"\\"+bvid+'.mp3')
        downloader = BilibiliDownloader(bvid, dirname = save_dir_name)
        if is_download_collection == False:
            data = downloader.get_html('https://www.bilibili.com/video/'+str(bvid)).text
            downloader.parser(data)
            downloader.download_data()
            print('音频下载完成!')
        else:
            num = downloader.get_collection_num()
            if num == -1:
                print('警告:该音频合集并不存在,您是否想下载单个音频?')
                pause()
                continue
            print(Style.RESET_ALL)
            print('本合集共'+str(num)+'个音频,')
            for i in range(1,num + 1):
                print('正在下载第'+str(i)+'个音频')
                data = downloader.get_html('https://www.bilibili.com/video/' + str(bvid) + '?p=' + str(i)).text
                downloader.parser(data)
                fname=downloader.get_fname(data,i)
                downloader.download_data(collection_name=fname)
            print('合集下载完成!')
        pause()

if __name__ == '__main__':
    main()
