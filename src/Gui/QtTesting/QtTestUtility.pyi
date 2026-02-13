# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
from typing import Final


@export(
    Include="Gui/QtTestUtility.h",
    Namespace="QtTesting",
    Constructor=False,
    Delete=False,
)
class QtTestUtility(BaseClass):
    """
    Interface to the QtTestUtility.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    @staticmethod
    def play(file : str | List[str]) -> Bool:
        """
        Playback a previously recorded test file or list of .xml files.

        Returns True if the tests were successful
        """

    @staticmethod
    def playingTest() -> Bool:
        """
        Indicates if the utility is currently playing a test
        """

    @staticmethod
    def stopTests() -> None:
        """
        Stop test playback
        """

    @staticmethod
    def record(file : str | None = None) -> None:
        """
        Open the recorder interface and save events in the specified .xml file
        """

    @staticmethod
    def stopRecording() -> None:
        """
        Stop recording.
        """

    @staticmethod
    def pauseRecording() -> None:
        """
        Pause recording.
        """

    @staticmethod
    def resumeRecording() -> None:
        """
        Resume recording after a pause.
        """
