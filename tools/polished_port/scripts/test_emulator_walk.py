#!/usr/bin/env python3
"""Controller-route regression controls for moving actors and real collisions."""
import unittest
from unittest.mock import patch

import test_new_bark_emulator as navigation


class Game:
    def __init__(self, moving=False, permanent=False):
        self.pos = (0, 1)
        self.live = {}
        self.moving, self.permanent = moving, permanent
        self.attempts = 0

    def position(self):
        return self.pos

    def step(self, key):
        self.attempts += 1
        dx, dy = next((dx, dy) for direction, dx, dy in navigation.DIRECTIONS if direction == key)
        target = self.pos[0] + dx, self.pos[1] + dy
        if self.attempts == 1 or self.permanent:
            if self.moving:
                self.live = {1: dict(position=target, invisible=False)}
            raise AssertionError(f"Blocked at {self.pos}, direction {key}")
        self.pos = target


class WalkTest(unittest.TestCase):
    def setUp(self):
        self.data = dict(name='Patrol crossing', width=3, height=3,
            cells=[3 << 12] * 9, warp_events=[],
            object_events=[dict(movement_range_x=0, movement_range_y=0)])
        self.mock = patch.object(navigation, 'actors', lambda game: game.live)
        self.mock.start()
        self.addCleanup(self.mock.stop)

    def test_new_actor_on_next_tile_replans_and_reaches_destination(self):
        game = Game(moving=True)
        navigation.walk(game, self.data, (2, 1))
        self.assertEqual(game.pos, (2, 1))
        self.assertEqual(game.attempts, 5)  # blocked step, then four real steps around it

    def test_static_collision_is_not_retried_or_hidden(self):
        game = Game()
        with self.assertRaisesRegex(AssertionError, 'Blocked at'):
            navigation.walk(game, self.data, (2, 1))
        self.assertEqual(game.attempts, 1)

    def test_actor_blocking_destination_cannot_retry_forever(self):
        game = Game(moving=True, permanent=True)
        with self.assertRaisesRegex(AssertionError, 'Blocked at'):
            navigation.walk(game, self.data, (1, 1))
        self.assertEqual(game.attempts, 9)


if __name__ == '__main__':
    unittest.main()
