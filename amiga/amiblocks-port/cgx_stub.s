SECTION code,CODE
    XDEF _cgx_WritePixelArray
    XREF _CyberGfxBase

_cgx_WritePixelArray:
    move.l  d2,-(a7)
    move.l  d3,-(a7)
    move.l  d4,-(a7)
    move.l  d5,-(a7)
    move.l  d6,-(a7)
    move.l  d7,-(a7)
    move.l  a2,-(a7)
    move.l  a6,-(a7)

    move.l  36(a7),a0
    moveq   #0,d0
    moveq   #0,d1
    move.l  40(a7),d2
    move.l  44(a7),a1
    move.l  48(a7),d3
    move.l  52(a7),d4
    move.l  56(a7),d5
    move.l  60(a7),d6
    moveq   #1,d7
    move.l  _CyberGfxBase,a6
    jsr     -126(a6)

    move.l  (a7)+,a6
    move.l  (a7)+,a2
    move.l  (a7)+,d7
    move.l  (a7)+,d6
    move.l  (a7)+,d5
    move.l  (a7)+,d4
    move.l  (a7)+,d3
    move.l  (a7)+,d2
    rts
    END
