CREATE TABLE deletions (
  id VARCHAR(10) NOT NULL PRIMARY KEY,
  type VARCHAR NOT NULL,
  resource_id VARCHAR(10) NOT NULL,
  system_updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
