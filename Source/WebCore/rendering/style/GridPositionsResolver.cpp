/*
 * Copyright (C) 2014-2016 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GridPositionsResolver.h"

#if ENABLE(CSS_GRID_LAYOUT)

#include "GridArea.h"
#include "RenderBox.h"
#include <cstdlib>

namespace WebCore {

static inline bool isColumnSide(GridPositionSide side)
{
    return side == ColumnStartSide || side == ColumnEndSide;
}

static inline bool isStartSide(GridPositionSide side)
{
    return side == ColumnStartSide || side == RowStartSide;
}

static inline GridPositionSide initialPositionSide(GridTrackSizingDirection direction)
{
    return direction == ForColumns ? ColumnStartSide : RowStartSide;
}

static inline GridPositionSide finalPositionSide(GridTrackSizingDirection direction)
{
    return direction == ForColumns ? ColumnEndSide : RowEndSide;
}

static inline GridTrackSizingDirection directionFromSide(GridPositionSide side)
{
    return side == ColumnStartSide || side == ColumnEndSide ? ForColumns : ForRows;
}

static const String implicitNamedGridLineForSide(const String& lineName, GridPositionSide side)
{
    return lineName + (isStartSide(side) ? "-start" : "-end");
}

NamedLineCollection::NamedLineCollection(const RenderStyle& gridContainerStyle, const String& namedLine, GridTrackSizingDirection direction, unsigned lastLine, unsigned autoRepeatTracksCount)
    : m_lastLine(lastLine)
    , m_repetitions(autoRepeatTracksCount)
{
    bool isRowAxis = direction == ForColumns;
    const NamedGridLinesMap& gridLineNames = isRowAxis ? gridContainerStyle.namedGridColumnLines() : gridContainerStyle.namedGridRowLines();
    const NamedGridLinesMap& autoRepeatGridLineNames = isRowAxis ? gridContainerStyle.autoRepeatNamedGridColumnLines() : gridContainerStyle.autoRepeatNamedGridRowLines();

    auto linesIterator = gridLineNames.find(namedLine);
    m_namedLinesIndexes = linesIterator == gridLineNames.end() ? nullptr : &linesIterator->value;

    auto autoRepeatLinesIterator = autoRepeatGridLineNames.find(namedLine);
    m_autoRepeatNamedLinesIndexes = autoRepeatLinesIterator == autoRepeatGridLineNames.end() ? nullptr : &autoRepeatLinesIterator->value;

    m_insertionPoint = isRowAxis ? gridContainerStyle.gridAutoRepeatColumnsInsertionPoint() : gridContainerStyle.gridAutoRepeatRowsInsertionPoint();
}

bool NamedLineCollection::isValidNamedLineOrArea(const String& namedLine, const RenderStyle& gridContainerStyle, GridPositionSide side)
{
    bool isRowAxis = directionFromSide(side) == ForColumns;
    auto& gridLineNames = isRowAxis ? gridContainerStyle.namedGridColumnLines() : gridContainerStyle.namedGridRowLines();
    auto& autoRepeatGridLineNames = isRowAxis ? gridContainerStyle.autoRepeatNamedGridColumnLines() : gridContainerStyle.autoRepeatNamedGridRowLines();

    if (gridLineNames.contains(namedLine) || autoRepeatGridLineNames.contains(namedLine))
        return true;

    String implicitName = implicitNamedGridLineForSide(namedLine, side);
    return gridLineNames.contains(implicitName) || autoRepeatGridLineNames.contains(implicitName);
}

bool NamedLineCollection::hasNamedLines() const
{
    return m_namedLinesIndexes || m_autoRepeatNamedLinesIndexes;
}

size_t NamedLineCollection::find(unsigned line) const
{
    if (line > m_lastLine)
        return notFound;

    if (!m_autoRepeatNamedLinesIndexes || line < m_insertionPoint)
        return m_namedLinesIndexes ? m_namedLinesIndexes->find(line) : notFound;

    if (line <= (m_insertionPoint + m_repetitions)) {
        size_t localIndex = line - m_insertionPoint;

        // The line names defined in the last line are also present in the first line of the next
        // repetition (if any). Same for the line names defined in the first line. Note that there
        // is only one auto-repeated track allowed by the syntax, that's why it's enough to store
        // indexes 0 and 1 (before and after the track size).
        if (localIndex == m_repetitions)
            return m_autoRepeatNamedLinesIndexes->find(1u);
        size_t position = m_autoRepeatNamedLinesIndexes->find(0u);
        if (position != notFound)
            return position;
        return localIndex ? m_autoRepeatNamedLinesIndexes->find(1u) : notFound;
    }

    return m_namedLinesIndexes ? m_namedLinesIndexes->find(line - (m_repetitions - 1)) : notFound;
}

bool NamedLineCollection::contains(unsigned line) const
{
    ASSERT(hasNamedLines());
    return find(line) != notFound;
}

unsigned NamedLineCollection::firstPosition() const
{
    ASSERT(hasNamedLines());
    unsigned firstLine = 0;

    if (!m_autoRepeatNamedLinesIndexes) {
        if (!m_insertionPoint || m_insertionPoint < m_namedLinesIndexes->at(firstLine))
            return m_namedLinesIndexes->at(firstLine) + (m_repetitions ? m_repetitions - 1 : 0);
        return m_namedLinesIndexes->at(firstLine);
    }

    if (!m_namedLinesIndexes)
        return m_autoRepeatNamedLinesIndexes->at(firstLine) + m_insertionPoint;

    if (!m_insertionPoint)
        return m_autoRepeatNamedLinesIndexes->at(firstLine);

    return std::min(m_namedLinesIndexes->at(firstLine), m_autoRepeatNamedLinesIndexes->at(firstLine) + m_insertionPoint);
}

static void adjustGridPositionsFromStyle(const RenderStyle& gridContainerStyle, const RenderBox& gridItem, GridTrackSizingDirection direction, GridPosition& initialPosition, GridPosition& finalPosition)
{
    bool isForColumns = direction == ForColumns;
    initialPosition = isForColumns ? gridItem.style().gridItemColumnStart() : gridItem.style().gridItemRowStart();
    finalPosition = isForColumns ? gridItem.style().gridItemColumnEnd() : gridItem.style().gridItemRowEnd();

    // We must handle the placement error handling code here instead of in the StyleAdjuster because we don't want to
    // overwrite the specified values.
    if (initialPosition.isSpan() && finalPosition.isSpan())
        finalPosition.setAutoPosition();

    if (gridItem.isOutOfFlowPositioned()) {
        // Early detect the case of non existing named grid lines for positioned items.
        if (initialPosition.isNamedGridArea() && !NamedLineCollection::isValidNamedLineOrArea(initialPosition.namedGridLine(), gridContainerStyle, initialPositionSide(direction)))
            initialPosition.setAutoPosition();

        if (finalPosition.isNamedGridArea() && !NamedLineCollection::isValidNamedLineOrArea(finalPosition.namedGridLine(), gridContainerStyle, finalPositionSide(direction)))
            finalPosition.setAutoPosition();
    }

    // If the grid item has an automatic position and a grid span for a named line in a given dimension, instead treat the grid span as one.
    if (initialPosition.isAuto() && finalPosition.isSpan() && !finalPosition.namedGridLine().isNull())
        finalPosition.setSpanPosition(1, String());
    if (finalPosition.isAuto() && initialPosition.isSpan() && !initialPosition.namedGridLine().isNull())
        initialPosition.setSpanPosition(1, String());
}

unsigned GridPositionsResolver::explicitGridColumnCount(const RenderStyle& gridContainerStyle, unsigned autoRepeatTracksCount)
{
    return std::min<unsigned>(std::max(gridContainerStyle.gridColumns().size() + autoRepeatTracksCount, gridContainerStyle.namedGridAreaColumnCount()), kGridMaxTracks);
}

unsigned GridPositionsResolver::explicitGridRowCount(const RenderStyle& gridContainerStyle, unsigned autoRepeatTracksCount)
{
    return std::min<unsigned>(std::max(gridContainerStyle.gridRows().size() + autoRepeatTracksCount, gridContainerStyle.namedGridAreaRowCount()), kGridMaxTracks);
}

static unsigned explicitGridSizeForSide(const RenderStyle& gridContainerStyle, GridPositionSide side, unsigned autoRepeatTracksCount)
{
    return isColumnSide(side) ? GridPositionsResolver::explicitGridColumnCount(gridContainerStyle, autoRepeatTracksCount) : GridPositionsResolver::explicitGridRowCount(gridContainerStyle, autoRepeatTracksCount);
}

static unsigned lookAheadForNamedGridLine(int start, unsigned numberOfLines, unsigned gridLastLine, NamedLineCollection& linesCollection)
{
    ASSERT(numberOfLines);

    // Only implicit lines on the search direction are assumed to have the given name, so we can start to look from first line.
    // See: https://drafts.csswg.org/css-grid/#grid-placement-span-int
    unsigned end = std::max(start, 0);

    if (!linesCollection.hasNamedLines())
        return std::max(end, gridLastLine + 1) + numberOfLines - 1;

    for (; numberOfLines; ++end) {
        if (end > gridLastLine || linesCollection.contains(end))
            numberOfLines--;
    }

    ASSERT(end);
    return end - 1;
}

static int lookBackForNamedGridLine(int end, unsigned numberOfLines, int gridLastLine, NamedLineCollection& linesCollection)
{
    ASSERT(numberOfLines);

    // Only implicit lines on the search direction are assumed to have the given name, so we can start to look from last line.
    // See: https://drafts.csswg.org/css-grid/#grid-placement-span-int
    int start = std::min(end, gridLastLine);

    if (!linesCollection.hasNamedLines())
        return std::min(start, -1) - numberOfLines + 1;

    for (; numberOfLines; --start) {
        if (start < 0 || linesCollection.contains(start))
            numberOfLines--;
    }

    return start + 1;
}

static int resolveNamedGridLinePositionFromStyle(const RenderStyle& gridContainerStyle, const GridPosition& position, GridPositionSide side, unsigned autoRepeatTracksCount)
{
    ASSERT(!position.namedGridLine().isNull());

    unsigned lastLine = explicitGridSizeForSide(gridContainerStyle, side, autoRepeatTracksCount);
    NamedLineCollection linesCollection(gridContainerStyle, position.namedGridLine(), directionFromSide(side), lastLine, autoRepeatTracksCount);

    if (position.isPositive())
        return lookAheadForNamedGridLine(0, std::abs(position.integerPosition()), lastLine, linesCollection);
    return lookBackForNamedGridLine(lastLine, std::abs(position.integerPosition()), lastLine, linesCollection);
}

static GridSpan definiteGridSpanWithNamedLineSpanAgainstOpposite(int oppositeLine, const GridPosition& position, GridPositionSide side, unsigned lastLine, NamedLineCollection& linesCollection)
{
    int start, end;
    if (side == RowStartSide || side == ColumnStartSide) {
        start = lookBackForNamedGridLine(oppositeLine - 1, position.spanPosition(), lastLine, linesCollection);
        end = oppositeLine;
    } else {
        start = oppositeLine;
        end = lookAheadForNamedGridLine(oppositeLine + 1, position.spanPosition(), lastLine, linesCollection);
    }

    return GridSpan::untranslatedDefiniteGridSpan(start, end);
}

static GridSpan resolveNamedGridLinePositionAgainstOppositePosition(const RenderStyle& gridContainerStyle, int oppositeLine, const GridPosition& position, GridPositionSide side, unsigned autoRepeatTracksCount)
{
    ASSERT(position.isSpan());
    ASSERT(!position.namedGridLine().isNull());
    // Negative positions are not allowed per the specification and should have been handled during parsing.
    ASSERT(position.spanPosition() > 0);

    unsigned lastLine = explicitGridSizeForSide(gridContainerStyle, side, autoRepeatTracksCount);
    NamedLineCollection linesCollection(gridContainerStyle, position.namedGridLine(), directionFromSide(side), lastLine, autoRepeatTracksCount);
    return definiteGridSpanWithNamedLineSpanAgainstOpposite(oppositeLine, position, side, lastLine, linesCollection);
}

static GridSpan resolveGridPositionAgainstOppositePosition(const RenderStyle& gridContainerStyle, int oppositeLine, const GridPosition& position, GridPositionSide side, unsigned autoRepeatTracksCount)
{
    if (position.isAuto()) {
        if (isStartSide(side))
            return GridSpan::untranslatedDefiniteGridSpan(oppositeLine - 1, oppositeLine);
        return GridSpan::untranslatedDefiniteGridSpan(oppositeLine, oppositeLine + 1);
    }

    ASSERT(position.isSpan());
    ASSERT(position.spanPosition() > 0);

    if (!position.namedGridLine().isNull()) {
        // span 2 'c' -> we need to find the appropriate grid line before / after our opposite position.
        return resolveNamedGridLinePositionAgainstOppositePosition(gridContainerStyle, oppositeLine, position, side, autoRepeatTracksCount);
    }

    // 'span 1' is contained inside a single grid track regardless of the direction.
    // That's why the CSS span value is one more than the offset we apply.
    unsigned positionOffset = position.spanPosition();
    if (isStartSide(side))
        return GridSpan::untranslatedDefiniteGridSpan(oppositeLine - positionOffset, oppositeLine);

    return GridSpan::untranslatedDefiniteGridSpan(oppositeLine, oppositeLine + positionOffset);
}

unsigned GridPositionsResolver::spanSizeForAutoPlacedItem(const RenderStyle& gridContainerStyle, const RenderBox& gridItem, GridTrackSizingDirection direction)
{
    GridPosition initialPosition, finalPosition;
    adjustGridPositionsFromStyle(gridContainerStyle, gridItem, direction, initialPosition, finalPosition);

    // This method will only be used when both positions need to be resolved against the opposite one.
    ASSERT(initialPosition.shouldBeResolvedAgainstOppositePosition() && finalPosition.shouldBeResolvedAgainstOppositePosition());

    if (initialPosition.isAuto() && finalPosition.isAuto())
        return 1;

    GridPosition position = initialPosition.isSpan() ? initialPosition : finalPosition;
    ASSERT(position.isSpan());

    ASSERT(position.spanPosition());
    return position.spanPosition();
}

static int resolveGridPositionFromStyle(const RenderStyle& gridContainerStyle, const GridPosition& position, GridPositionSide side, unsigned autoRepeatTracksCount)
{
    switch (position.type()) {
    case ExplicitPosition: {
        ASSERT(position.integerPosition());

        if (!position.namedGridLine().isNull())
            return resolveNamedGridLinePositionFromStyle(gridContainerStyle, position, side, autoRepeatTracksCount);

        // Handle <integer> explicit position.
        if (position.isPositive())
            return position.integerPosition() - 1;

        unsigned resolvedPosition = std::abs(position.integerPosition()) - 1;
        const unsigned endOfTrack = explicitGridSizeForSide(gridContainerStyle, side, autoRepeatTracksCount);

        return endOfTrack - resolvedPosition;
    }
    case NamedGridAreaPosition:
    {
        // First attempt to match the grid area's edge to a named grid area: if there is a named line with the name
        // ''<custom-ident>-start (for grid-*-start) / <custom-ident>-end'' (for grid-*-end), contributes the first such
        // line to the grid item's placement.
        String namedGridLine = position.namedGridLine();
        ASSERT(!position.namedGridLine().isNull());

        unsigned lastLine = explicitGridSizeForSide(gridContainerStyle, side, autoRepeatTracksCount);
        NamedLineCollection implicitLines(gridContainerStyle, implicitNamedGridLineForSide(namedGridLine, side), directionFromSide(side), lastLine, autoRepeatTracksCount);
        if (implicitLines.hasNamedLines())
            return implicitLines.firstPosition();

        // Otherwise, if there is a named line with the specified name, contributes the first such line to the grid
        // item's placement.
        NamedLineCollection explicitLines(gridContainerStyle, namedGridLine, directionFromSide(side), lastLine, autoRepeatTracksCount);
        if (explicitLines.hasNamedLines())
            return explicitLines.firstPosition();

        ASSERT(!NamedLineCollection::isValidNamedLineOrArea(namedGridLine, gridContainerStyle, side));
        // If none of the above works specs mandate to assume that all the lines in the implicit grid have this name.
        return lastLine + 1;
    }
    case AutoPosition:
    case SpanPosition:
        // 'auto' and span depend on the opposite position for resolution (e.g. grid-row: auto / 1 or grid-column: span 3 / "myHeader").
        ASSERT_NOT_REACHED();
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

GridSpan GridPositionsResolver::resolveGridPositionsFromStyle(const RenderStyle& gridContainerStyle, const RenderBox& gridItem, GridTrackSizingDirection direction, unsigned autoRepeatTracksCount)
{
    GridPosition initialPosition, finalPosition;
    adjustGridPositionsFromStyle(gridContainerStyle, gridItem, direction, initialPosition, finalPosition);

    GridPositionSide initialSide = initialPositionSide(direction);
    GridPositionSide finalSide = finalPositionSide(direction);

    // We can't get our grid positions without running the auto placement algorithm.
    if (initialPosition.shouldBeResolvedAgainstOppositePosition() && finalPosition.shouldBeResolvedAgainstOppositePosition())
        return GridSpan::indefiniteGridSpan();

    if (initialPosition.shouldBeResolvedAgainstOppositePosition()) {
        // Infer the position from the final position ('auto / 1' or 'span 2 / 3' case).
        auto endLine = resolveGridPositionFromStyle(gridContainerStyle, finalPosition, finalSide, autoRepeatTracksCount);
        return resolveGridPositionAgainstOppositePosition(gridContainerStyle, endLine, initialPosition, initialSide, autoRepeatTracksCount);
    }

    if (finalPosition.shouldBeResolvedAgainstOppositePosition()) {
        // Infer our position from the initial position ('1 / auto' or '3 / span 2' case).
        auto startLine = resolveGridPositionFromStyle(gridContainerStyle, initialPosition, initialSide, autoRepeatTracksCount);
        return resolveGridPositionAgainstOppositePosition(gridContainerStyle, startLine, finalPosition, finalSide, autoRepeatTracksCount);
    }

    int startLine = resolveGridPositionFromStyle(gridContainerStyle, initialPosition, initialSide, autoRepeatTracksCount);
    int endLine = resolveGridPositionFromStyle(gridContainerStyle, finalPosition, finalSide, autoRepeatTracksCount);

    if (startLine > endLine)
        std::swap(startLine, endLine);
    else if (startLine == endLine)
        endLine = startLine + 1;

    return GridSpan::untranslatedDefiniteGridSpan(startLine, std::max(startLine, endLine));
}

} // namespace WebCore

#endif // ENABLE(CSS_GRID_LAYOUT)
