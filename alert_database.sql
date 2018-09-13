ALTER TABLE nodes RENAME TO sqlitestudio_temp_table;

CREATE TABLE nodes (node_id VARCHAR (120), user_id VARCHAR (120), node_sn VARCHAR (120), name VARCHAR (256), private_key VARCHAR (120), dataxserver VARCHAR (1024), board VARCHAR (256));

INSERT INTO nodes (node_id, user_id, node_sn, name, private_key, dataxserver, board) SELECT node_id, user_id, node_sn, name, private_key, dataxserver, board FROM sqlitestudio_temp_table;

DROP TABLE sqlitestudio_temp_table;

CREATE INDEX private_key_index ON nodes (private_key);



ALTER TABLE resources RENAME TO sqlitestudio_temp_table;

CREATE TABLE resources (res_id INTEGER PRIMARY KEY AUTOINCREMENT, node_id VARCHAR (120), chksum_config VARCHAR (120), chksum_dbjson VARCHAR (120), render_content TEXT);

INSERT INTO resources (res_id, node_id, chksum_config, chksum_dbjson, render_content) SELECT res_id, node_id, chksum_config, chksum_dbjson, render_content FROM sqlitestudio_temp_table;

DROP TABLE sqlitestudio_temp_table;

CREATE INDEX node_id_index ON resources (node_id);



ALTER TABLE users RENAME TO sqlitestudio_temp_table;

CREATE TABLE users (user_id VARCHAR (120), email VARCHAR (256), pwd VARCHAR (256), token VARCHAR (120), ext_bind_id VARCHAR (120), ext_bind_region VARCHAR (120), created_at DATETIME);

INSERT INTO users (user_id, email, pwd, token, ext_bind_id, ext_bind_region, created_at) SELECT user_id, email, pwd, token, ext_bind_id, ext_bind_region, created_at FROM sqlitestudio_temp_table;

DROP TABLE sqlitestudio_temp_table;

CREATE INDEX token_index ON users (token);

