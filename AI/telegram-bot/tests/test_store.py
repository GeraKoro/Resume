from unittest.mock import AsyncMock, Mock

import pytest

from app.store import PostgresDialogStore, StoreError


async def test_invalid_temperature_is_not_written():
    # Arrange
    pool = Mock(execute=AsyncMock())
    store = PostgresDialogStore(pool)
    # Act / Assert
    with pytest.raises(StoreError, match="temperature"):
        await store.set_temperature(7, "0.5")
    pool.execute.assert_not_called()


async def test_unknown_mode_is_not_written():
    # Arrange
    pool = Mock(execute=AsyncMock())
    store = PostgresDialogStore(pool)
    # Act / Assert
    with pytest.raises(StoreError, match="режим"):
        await store.set_mode(7, "chat")
    pool.execute.assert_not_called()


async def test_unknown_role_is_not_written():
    # Arrange
    pool = Mock(execute=AsyncMock())
    store = PostgresDialogStore(pool)
    # Act / Assert
    with pytest.raises(StoreError, match="роль"):
        await store.add_message(7, "system", "инструкция")
    pool.execute.assert_not_called()
