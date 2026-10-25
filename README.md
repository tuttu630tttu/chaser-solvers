# chaser-solvers

CHaser用のsolverとマップの置き場。コードはローカルで編集し、対戦はWeb上で行います。

- `2025_solver.cpp`: 2025年のsolver
- `CHaser.hpp`: ローカル開発用のヘッダ
- `mapjson/`: Web版で読み込むマップ

このリポジトリには対戦サーバーは含まれていません。

## ログの自動命名

次のコマンドを起動したまま、対戦ログのJSONを `logs/` に保存します。停止は Ctrl+C。

```sh
python3 tools/watch_logs.py
```

ダウンロード後にファイルのサイズ・更新日時が2秒間変わらず、JSONとして読み込めたら、
`DATE_TIME_COOL_HOT_BOARD_TURN_N.json` に変更します。日時はログの `executedAt` を日本時間に変換し、TURNは設定した上限ターン数です。
同じCOOL・HOT・ボード・上限ターン数の組み合わせごとに、末尾のNを1から増やします。日付が変わっても監視を再起動しても、保存済みログの最大番号の続きになります。先後を入れ替えた組み合わせは別カウントです。番号付きの既存ファイルは変更せず、番号のない旧形式のログも命名対象になります。JSONの内容は変更しません。

プログラムIDと名前の対応は `tools/log_names.json` で設定します。ボードは対応表、または `mapjson/` と初期盤面の一致から名前を決めます。
未知のIDは `program-ID` / `board-ID` として保存します。新しいsolverを登録したら対応表にも追加してください。
起動時に存在する未命名ログも処理します。一度だけまとめて処理する場合は `python3 tools/watch_logs.py --once`。

solverの変更内容と対戦ログ番号の対応は [変更メモ](logs/solver_notes.md) に記録します。

## solverのデバッグ出力

`2026_solver.cpp` は起動時に `solver-build: enemy-memory-v1` を表示します。
Webへ貼り替えた後はこの行で版を確認できます。

例: `decision=EXPLORE target=(-3,2) distance=5 unknown=4 direction=1`

- `ITEM`: 既知アイテムへ移動
- `EXPLORE`: 未知の観測効率で選んだ目的地へ移動
- `FALLBACK`: アイテムも未知の観測候補もない場合の移動
- `PUT`: 隣接敵への攻撃
- `SEARCH_WAIT`: 危険な接近・アイテム取得による閉じ込めを避けてsearchで待機
- `NO_MOVE`: 歩けるマスがなく終了

目的地と現在位置は出発点基準の相対座標、距離は現在地からのBFS歩数です。
方向番号は0=上、1=左、2=右、3=下。`unknown`は目的地周囲3×3の未知マス数、
`unknown_line`はsearch対象の直線9マスの未知数です。search待機には移動目的地はありません。

敵を見失ってから3ターンは候補位置を保持します。`enemy_memory: age=1 candidates=4` のように警戒期間と候補数を出力します（age=-1は警戒なし、0は敵を観測したターン）。
