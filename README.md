
# MicoLisp

自家製のLisp処理系（方言）です。

前回書いていたLisp処理系が複雑になりすぎて頓挫したため、逆に簡素な設計を目指してみました。
簡素な設計なので取り扱える型は「数値（倍精度浮動小数点数）」「シンボル」「関数」「リスト」の四種類のみです。
文字列リテラルに対応していますが、文字列型はありません。
それらは数値のリストとして解釈されます。
またマルチバイト文字には対応していません。

```lisp
12345 ;; number 
'abc  ;; symbol
"abc" ;; list
'(1 2 . 3) ;; list
(function (a) (+ a 1)) ;; function
```

Lispなので当然マクロも使えます。
ただし、バッククオートには対応していないので少々面倒です。
また、読み込み処理を簡素化するためにリーダマクロには未対応です。

```lisp
(macro example (a b)
  (list 'progn b a))
(example 1 2) ;; 1
```

汎変数にも対応しています。
`set`関数を使うことで評価結果に値を代入することができます。

```lisp
(var n '(1 2 3))
(set (nth 1 n) nil)
n ;; (1 nil 3)
```

GCは参照カウント方式を採用しています。
GCが正常に動作しているか不明なのでメモリリーク関連のテストコードも増やしていきたいです。

この処理系は趣味で書いたついでに公開しているだけあり、
不具合の発見・修正も楽しみのひとつなので、
不具合報告は受け付けておりません。
困ったときには自力で解決してください。

## Author's Environments

* Windows 10 
* gcc (MinGW.org GCC-6.3.0-1) 6.3.0
* GNU gdb (GDB) 7.6.1

## Build MicoLisp

```
make setup
make release 
make test 
```

## License 

MicoLisp released under the [MIT License](LICENSE).
