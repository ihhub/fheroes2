passable:
    TopLeft = 0x0001, Top = 0x0002, TopRight = 0x0004, TopRow  = 0x0007,
    Right = 0x0008, Left = 0x0080, Center = 0x0100, CenterRow = 0x0188,
    BottomRight = 0x0010, Bottom = 0x0020, BottomLeft = 0x0040, BottomRow = 0x0070

    CenterRow + TopRow = 0x018F
    CenterRow + BottomRow = 0x01F8
    All = 0x01FF
