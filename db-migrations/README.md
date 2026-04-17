# db-migrations

This directory owns PostgreSQL schema evolution for Chronos.

Runtime services depend on the schema, but migration files remain isolated from
runtime application logic.
