// Copyright 2021 Team 254. All Rights Reserved.
// Author: pat@patfairbank.com (Patrick Fairbank)

package model

import (
	"github.com/stretchr/testify/assert"
	"testing"
)

type validRecord struct {
	Id         int `db:"id"`
	IntData    int
	StringData string
}

type manualIdRecord struct {
	Id         int `db:"id,manual""`
	StringData string
}

func TestTableSingleCrud(t *testing.T) {
	db := setupTestDb(t)
	defer db.Close()

	table, err := newTable[validRecord](db)
	if !assert.Nil(t, err) {
		return
	}

	// Test initial create and then read back.
	record := validRecord{IntData: 254, StringData: "The Cheesy Poofs"}
	if assert.Nil(t, table.create(&record)) {
		assert.Equal(t, 1, record.Id)
	}
	record2, err := table.getById(record.Id)
	assert.Equal(t, record, *record2)
	assert.Nil(t, err)

	// Test update and then read back.
	record.IntData = 252
	record.StringData = "Teh Chezy Pofs"
	assert.Nil(t, table.update(&record))
	record2, err = table.getById(record.Id)
	assert.Equal(t, record, *record2)
	assert.Nil(t, err)

	// Test delete.
	assert.Nil(t, table.delete(record.Id))
	record2, err = table.getById(record.Id)
	assert.Nil(t, record2)
	assert.Nil(t, err)
}

func TestTableMultipleCrud(t *testing.T) {
	db := setupTestDb(t)
	defer db.Close()

	table, err := newTable[validRecord](db)
	if !assert.Nil(t, err) {
		return
	}

	// Insert a few test records.
	record1 := validRecord{IntData: 1, StringData: "One"}
	record2 := validRecord{IntData: 2, StringData: "Two"}
	record3 := validRecord{IntData: 3, StringData: "Three"}
	assert.Nil(t, table.create(&record1))
	assert.Nil(t, table.create(&record2))
	assert.Nil(t, table.create(&record3))

	// Read all records.
	records, err := table.getAll()
	assert.Nil(t, err)
	if assert.Equal(t, 3, len(records)) {
		assert.Equal(t, record1, records[0])
		assert.Equal(t, record2, records[1])
		assert.Equal(t, record3, records[2])
	}

	// Truncate the table and verify that the records no longer exist.
	assert.Nil(t, table.truncate())
	records, err = table.getAll()
	assert.Equal(t, 0, len(records))
	assert.Nil(t, err)
	record4, err := table.getById(record1.Id)
	assert.Nil(t, record4)
	assert.Nil(t, err)
}

func TestTableWithManualId(t *testing.T) {
	db := setupTestDb(t)
	defer db.Close()

	table, err := newTable[manualIdRecord](db)
	if !assert.Nil(t, err) {
		return
	}

	// Test initial create and then read back.
	record := manualIdRecord{Id: 254, StringData: "The Cheesy Poofs"}
	if assert.Nil(t, table.create(&record)) {
		assert.Equal(t, 254, record.Id)
	}
	record2, err := table.getById(record.Id)
	assert.Equal(t, record, *record2)
	assert.Nil(t, err)

	// Test update and then read back.
	record.StringData = "Teh Chezy Pofs"
	assert.Nil(t, table.update(&record))
	record2, err = table.getById(record.Id)
	assert.Equal(t, record, *record2)
	assert.Nil(t, err)

	// Test delete.
	assert.Nil(t, table.delete(record.Id))
	record2, err = table.getById(record.Id)
	assert.Nil(t, record2)
	assert.Nil(t, err)

	// Test creating a record with a zero ID.
	record.Id = 0
	err = table.create(&record)
	if assert.NotNil(t, err) {
		assert.Equal(
			t, "can't create manualIdRecord with zero ID since table is configured for manual IDs", err.Error(),
		)
	}
}

func TestNewTableErrors(t *testing.T) {
	db := setupTestDb(t)
	defer db.Close()

	// Pass a non-struct as the record type.
	table, err := newTable[int](db)
	assert.Nil(t, table)
	if assert.NotNil(t, err) {
		assert.Equal(t, "record type must be a struct; got int", err.Error())
	}

	// Pass a struct that doesn't have an ID field.
	type recordWithNoId struct {
		StringData string
	}
	table2, err := newTable[recordWithNoId](db)
	assert.Nil(t, table2)
	if assert.NotNil(t, err) {
		assert.Equal(t, "struct recordWithNoId has no field tagged as the id", err.Error())
	}

	// Pass a struct that has a field with the wrong type tagged as the ID.
	type recordWithWrongIdType struct {
		Id bool `db:"id"`
	}
	table3, err := newTable[recordWithWrongIdType](db)
	assert.Nil(t, table3)
	if assert.NotNil(t, err) {
		assert.Equal(
			t, "field in struct recordWithWrongIdType tagged with 'id' must be an int; got bool", err.Error(),
		)
	}
}

func TestTableCrudErrors(t *testing.T) {
	db := setupTestDb(t)
	defer db.Close()

	table, err := newTable[validRecord](db)
	if !assert.Nil(t, err) {
		return
	}

	// Create a record with a non-zero ID.
	var record validRecord
	record.Id = 12345
	err = table.create(&record)
	if assert.NotNil(t, err) {
		assert.Equal(
			t,
			"can't create validRecord with non-zero ID since table is configured for autogenerated IDs: 12345",
			err.Error(),
		)
	}

	// Update a record with an ID of zero.
	record.Id = 0
	err = table.update(&record)
	if assert.NotNil(t, err) {
		assert.Equal(t, "can't update validRecord with zero ID", err.Error())
	}

	// Update a nonexistent record.
	record.Id = 12345
	err = table.update(&record)
	if assert.NotNil(t, err) {
		assert.Equal(t, "can't update non-existent validRecord with ID 12345", err.Error())
	}

	// Delete a nonexistent record.
	err = table.delete(12345)
	if assert.NotNil(t, err) {
		assert.Equal(t, "can't delete non-existent validRecord with ID 12345", err.Error())
	}
}