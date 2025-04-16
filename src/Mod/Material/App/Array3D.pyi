from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Any, Final, List


@export(
    Twin="Array3D",
    TwinPointer="Array3D",
    Namespace="Materials",
    Include="Mod/Material/App/MaterialValue.h",
    Delete=True,
    Constructor=True
)
class Array3D(BaseClass):
    """
    3D Array of material properties.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Array: Final[List] = ...
    """The 3 dimensional array."""

    Dimensions: Final[int] = ...
    """The number of dimensions in the array, in this case 3."""

    Columns: int = ...
    """The number of columns in the array."""

    Depth: int = ...
    """The depth of the array (3rd dimension)."""

    @constmethod
    def getRows(self) -> int:
        """
        Get the number of rows in the array at the specified depth. If no depth is
        specified it will use the last used depth
        """
        ...

    @constmethod
    def getRow(self, depth: int, row: int) -> List:
        """
        Get the row given the depth and row indices
        """
        ...

    @constmethod
    def getValue(self, depth: int, row: int, column: int) -> Any:
        """
        Get the value at the given depth, row, and column
        """
        ...

    @constmethod
    def getDepthValue(self, depth: int) -> Any:
        """
        Get the value for the given depth
        """
        ...

    def setDepthValue(self, depth: int, value: Any):
        """
        Set the value for the given depth
        """
        ...

    def setValue(self, depth: int, row: int, column: int, value: Any):
        """
        Set the value at the given depth, row, and column
        """
        ...

    def setRows(self, depth: int, value: int):
        """
        Set the number of rows at the given depth
        """
        ...
