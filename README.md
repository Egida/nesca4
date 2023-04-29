# nesca4
## Половина функций ещё не рабочая!!
Консольная утилита для сканирования `портов` и `dns`, брутфорсер для `ftp`, `sftp`, `hikvision` и `rvi`.
Поддерживает многопоточность: До `500` потоков работает стабильно, дальше за ваш процессор не ручаюсь.
Использует `TCP` сканирования, но поддерживает полную настройку его. Разогнать можно очень сильно.
Полностью кроссплотформлена, для компиляции на `linux` используйте `gcc` для `windows` `MinGW`.

![alt text](https://i.imgur.com/xoJQ2N5.png)

# UPDATE
+ Добавлена многопоточность для сканирования портов.
+ Добавлены оригинальные цвета с nesca3.
+ Настроен вывод.
+ Исправлена случайная остановка во время сканирования.
+ Добавлены цвета для DNS сканирования.
+ Добавлен аргумент -no-color.
+ Оптимизированы цвета.
+ Добавлены цвета в help menu.
+ Добавлено сохранение в txt для сканирования портов.
+ Настроен debug для сканирования поротов.
+ Добавлен брутфорс FTP
+ Добавлены filter ports в debug
+ Добавлен обход фаервола сканирования
+ Добавлены аргументы ftp-brute-log и ftp-brute-verbose

Все рабочие функции:
```
d8b   db d88888b .d8888.  .o88b.  .d8b.         j88D  
888o  88 88'     88'  YP d8P  Y8 d8' `8b       j8~88  
88V8o 88 88ooooo `8bo.   8P      88ooo88      j8' 88  
88 V8o88 88~~~~~   `Y8b. 8b      88~~~88      V88888D 
88  V888 88.     db   8D Y8b  d8 88   88          88  
VP   V8P Y88888P `8888Y'  `Y88P' YP   YP          VP  

usage: ./nesca4 [flags]

arguments target:
  -ip <1,2,3>            Set ip target.
  -cidr <1,2,3>          Set cidr target.
  -import-ip <path>      Set ips on target from file.
  -import-cidr <path>    Set cidr on target from file.
  -random-ip <count>     Set random ips target.

arguments ports:
  -ports, -p <1,2,3>  Set ports on scan.
     - example ports: all, nesca, top100, top50

arguments speed:
  -threads, -T <count>   Set threads for scan.
  -timeout, -t <ms>      Set timeout for scan.

arguments bruteforce:
  -no-ftp-brute          Off bruteforce ftp.
  -ftp-login             Set path for ftp logins.
  -ftp-pass              Set path for ftp passwords.
  -ftp-brute-log         Display bruteforce ftp info.
  -ftp-brute-verbose     Display bruteforce ftp all info.

arguments dns-scan:
  -dns-scan <.dns>       On dns-scan and set domain 1 level.
  -dns-length <count>    Edit length generating domain.
  -dns-dict <dict>       Edit dictionary for generation.

arguments output:
  -db, -debug            On debug mode, save and display not even working hosts.
  -er, -error            On display errors.
  -no-color              Disable colors.
  -log-set <count>       Change change the value of ips after which, will be displayed information about how much is left.
  -txt <path>            Save result to text document.
```

# Документация
## Цель
Для указания цели есть эти аргументы:

- `-ip` - Указать в качестве цели обычный айпи, можно указать несколько, через запятую без пробела.

- `-cidr` - Указать в качесте цели `cidr`, можно указать несоклько, через запятую без пробела.

- `-import-ip` - Указать в качесте цели файл, с обычными айпи, каждый новый айпи на новой строчке.

- `-import-cidr` - Указать в качесте цели файл, с `cidr`, каждый новый `cidr` на новой строчке.

Для генерации рандомной цели есть эти:

- `-random-ip` - Указать в качстве цели рандомные айпи, в качестве параметра передавайте количество рандомный айпи.

## Порты
Для указания портов есть эти аргументы:

- `-ports, -p` - Указать порты на сканирование, можно указать несоклько через запятую: `80,20,22,443`.
Также можно вместо цифры указать - `all` для сканирования всех портов, или - `100` для указания 
топ 100 самых популярных портов, или - `50` для указания топ 50 самый популярных портов.


## Потоки
Для работы с потоками есть эти аргументы:

- `-threads, -T` - Позволяет указать количество потоков, они пердаються в параметр аргумента.
- `-timeout, -t` - Позволяет изменить задержку, для сканирования, стандартно `70`.

## DNS сканирования
Для работы с `dns` сканированием есть эти аргументы:

- `-dns-scan` - Включает сканирование `dns`, в параметр принимает, домен первого уровня: **.ru .net .com**
- `-dns-length` - Изменить длинну генерируемого домена, стандартно `5`. Длинну указывать в параметр.
- `-dns-dict` - Позволяет изменить словарь, из которого генерируеться рандомная строка, стандартный: `0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`.
Словарь указывать в качестве параметра.

## Сохранение
Для сохраниня результатов можно использывать эти аргументы:

- `-txt` - Позволяет сохранять результат в `txt`, причём если включен `debug` то он будет записывать и не удачные результаты.
- `-html` - Аналогично `txt`, только в `html`.

## Вывод
Для изменения вывода программы используйте эти аргументы:

- `-db` - Включает `debug` при котором будет выводить и сохраняться даже неудачные цели.
- `-clean` - Выключает вывод вообще, но сохранение до сих пор работает.
- `-log-set` - Позволяет изменить значение просканированных айпи после каторого, будет выводиться информация
о том сколько осталось. Стандартно `100`.

## Настройки сканирования
Для настройки сканирования есть эти аргументы:

- `-ping-off` - Отключить определение времени ответа хоста.
- `-dns-off` - Отключить получение dns у портов 443, 80.

## Настройки брутфорса
Для настройки брутфорса используйте эти аргументы:

- `-ftp-brute` - Включить брутфорс `ftp`.
- `-ftp-login` - Изменить путь к файлу в котором храняться логины для брутфорса ftp. Стандартно стоит `passwd/ftp_logins.txt`
- `-ftp-pass` - Изменить путь к файлу в котором храняться пароли для брутфорса ftp. Стандартно стоит `passwd/ftp_passwords.txt`
- `-ssh-brute` - Включить брутфорс `ssh`.
- `-ssh-login` - Аналогично `ftp-login`, стандартно: `passwd/ssh_logins.txt`
- `-ssh-pass` - Аналогично `ftp-pass`, стандартно: `passwd/ssh_passwords`
