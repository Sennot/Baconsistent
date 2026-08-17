#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace baconsistent::core {

// Editable copy of a profile's paired 2.1 / 2.2 boundaries. The first and
// last entries are always 0 and 100 and cannot be moved. The editor changes
// physical marker positions by index so repetition counters stay attached to
// the same stages.
class StageLayoutDraft {
public:
    StageLayoutDraft() = default;
    StageLayoutDraft(std::vector<double> legacy21, std::vector<double> modern22)
      : m_original21(std::move(legacy21)),
        m_original22(std::move(modern22)),
        m_legacy21(m_original21),
        m_modern22(m_original22) {}

    [[nodiscard]] bool valid() const {
        if (m_legacy21.size() < 3 || m_legacy21.size() != m_modern22.size()) {
            return false;
        }
        if (std::abs(m_legacy21.front()) > 0.01 || std::abs(m_modern22.front()) > 0.01 ||
            std::abs(m_legacy21.back() - 100.0) > 0.01 || std::abs(m_modern22.back() - 100.0) > 0.01) {
            return false;
        }
        for (std::size_t i = 1; i < m_legacy21.size(); ++i) {
            if (!std::isfinite(m_legacy21[i]) || !std::isfinite(m_modern22[i]) ||
                m_legacy21[i] <= m_legacy21[i - 1] || m_modern22[i] <= m_modern22[i - 1]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::size_t markerCount() const {
        return m_legacy21.size() >= 2 ? m_legacy21.size() - 2 : 0;
    }

    [[nodiscard]] std::vector<double> const& legacy21() const { return m_legacy21; }
    [[nodiscard]] std::vector<double> const& modern22() const { return m_modern22; }
    [[nodiscard]] bool changed() const { return m_legacy21 != m_original21 || m_modern22 != m_original22; }

    bool moveMarker(std::size_t markerIndex, double legacy21, double modern22, double minGap = 0.05) {
        if (!valid() || markerIndex >= markerCount() || !std::isfinite(legacy21) || !std::isfinite(modern22)) {
            return false;
        }
        auto const index = markerIndex + 1;
        minGap = std::max(0.01, minGap);
        auto const legacyMin = m_legacy21[index - 1] + minGap;
        auto const legacyMax = m_legacy21[index + 1] - minGap;
        auto const modernMin = m_modern22[index - 1] + minGap;
        auto const modernMax = m_modern22[index + 1] - minGap;
        if (legacyMin >= legacyMax || modernMin >= modernMax) {
            return false;
        }
        m_legacy21[index] = std::clamp(legacy21, legacyMin, legacyMax);
        m_modern22[index] = std::clamp(modern22, modernMin, modernMax);
        return true;
    }

    void reset() {
        m_legacy21 = m_original21;
        m_modern22 = m_original22;
    }

private:
    std::vector<double> m_original21;
    std::vector<double> m_original22;
    std::vector<double> m_legacy21;
    std::vector<double> m_modern22;
};

} // namespace baconsistent::core
