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
