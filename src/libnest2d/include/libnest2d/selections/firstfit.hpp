#ifndef FIRSTFIT_HPP
#define FIRSTFIT_HPP

#include "selection_boilerplate.hpp"
// for writing SVG
// #include "../tools/svgtools.hpp"

namespace libnest2d { namespace selections {

template<class RawShape> class _FirstFitSelection : public SelectionBoilerplate<RawShape>
{
    using Base = SelectionBoilerplate<RawShape>;

public:
    using typename Base::Item;
    using Config = int; // dummy

private:
    using Base::packed_bins_;
    using typename Base::ItemGroup;
    using Container = ItemGroup; // typename std::vector<_Item<RawShape>>;

    Container store_;

public:
    void configure(const Config& /*config*/) {}

    template<class TPlacer,
             class TIterator,
             class TBin    = typename PlacementStrategyLike<TPlacer>::BinType,
             class PConfig = typename PlacementStrategyLike<TPlacer>::Config>
    void packItems(TIterator first, TIterator last, TBin&& bin, PConfig&& pconfig = PConfig())
    {
        using Placer = PlacementStrategyLike<TPlacer>;

        store_.clear();
        store_.reserve(last - first);

        std::vector<Placer> placers;
        placers.reserve(last - first);

        std::for_each(first, last, [this](Item& itm) {
            if (itm.isFixed()) {
                if (itm.binId() < 0)
                    itm.binId(0);
                auto binidx = size_t(itm.binId());

                while (packed_bins_.size() <= binidx) // 减少多次扩容的内存分配开销,1.5倍扩容
                    packed_bins_.emplace_back();

                packed_bins_[binidx].emplace_back(itm); // 按编号存放固定的零件
            } else {
                store_.emplace_back(itm); // 存放不固定的零件
            }
        });

        for (int i = 0; i < packed_bins_.size(); ++i) {
            const ItemGroup& ig = packed_bins_[i];
            placers.emplace_back(bin);
            placers.back().configure(pconfig);
            placers.back().preload(ig);
        }

        std::for_each(pconfig.m_excluded_regions.begin(), pconfig.m_excluded_regions.end(),
                      [this, &pconfig](Item& itm) { pconfig.m_excluded_items.emplace_back(itm); });

#ifdef SVGTOOLS_HPP
        svg::SVGWriter<RawShape> svgwriter;
        std::for_each(first, last, [this, &svgwriter](Item& itm) { svgwriter.writeShape(itm, "none", "blue"); });
        svgwriter.save(boost::filesystem::path("SVG") / "all_items.svg");
#endif

        std::function<bool(Item & i1, Item & i2)> sortfunc;
        if (pconfig.sortfunc)
            sortfunc = pconfig.sortfunc;
        else {
            sortfunc = [](Item& i1, Item& i2) {
                int p1 = i1.priority(), p2 = i2.priority();
                if (p1 != p2)
                    return p1 > p2;

                return i1.bed_temp != i2.bed_temp ? (i1.bed_temp > i2.bed_temp) :
                                                    (i1.height != i2.height ? (i1.height < i2.height) : (i1.area() > i2.area()));
            };
        }

        std::sort(store_.begin(), store_.end(), sortfunc);

        // debug: write down intitial order
        for (auto it = store_.begin(); it != store_.end(); ++it) {
            std::stringstream ss;
            ss << "initial order: " << it->get().name << ", p=" << it->get().priority() << ", bed_temp=" << it->get().bed_temp
               << ", height=" << it->get().height << ", area=" << it->get().area();
            if (this->unfitindicator_)
                this->unfitindicator_(ss.str());
        }

        int  item_id      = 0;
        auto makeProgress = [this, &item_id](Placer& placer, size_t bin_idx) {
            packed_bins_[bin_idx]     = placer.getItems(); // 获取packed_bins_[i]对应床号的零件
            this->last_packed_bin_id_ = int(bin_idx);      // 床号  大于0 是存放到虚拟床(即打印床外)
            this->progress_(static_cast<unsigned>(--item_id));
        };

        auto& cancelled = this->stopcond_;

        this->template remove_unpackable_items<Placer>(store_, bin, pconfig);

        auto it = store_.begin();
        while (it != store_.end() && !cancelled()) {
            bool                        was_packed = false; 
            size_t                      j = 0;
            typename Placer::PackResult result;
            double                      score = LARGE_COST_TO_REJECT + 1;
            while (!was_packed && !cancelled()) // 不能打包或取消
            {
                // placers为摆放的堆数，例如盘内+盘外两堆(盘内摆满时)， 或仅盘内一堆（盘内还有空间）
                for (; j < placers.size() && !was_packed && !cancelled(); ++j) {
                    // 调用PackResult pack(Item& item, const Range& rem = Range())进行打包，并返回结果
                    result = placers[j].pack(*it, rem(it, store_));
                    score  = result.score();
                    if (score >= 0 && score < LARGE_COST_TO_REJECT) {
                        was_packed = true;
                        it->get().binId(int(j));
                        placers[j].accept(result);
                        makeProgress(placers[j], j);
                    }
                }
                if (!was_packed) {
                    if (placers.empty()) {
                        placers.emplace_back(bin);
                        placers.back().configure(pconfig);
                        packed_bins_.emplace_back();
                        j = placers.size() - 1;
                    } else if (true) {
                        placers.emplace_back(bin);
                        placers.back().plateID(placers.size() - 1);
                        placers.back().configure(pconfig);
                        if (packed_bins_.size() >= placers.size())
                            placers.back().preload(packed_bins_[placers.size() - 1]);
                        // placers.back().preload(pconfig.m_excluded_items);
                        packed_bins_.emplace_back();
                        j = placers.size() - 1;
                    } else {
                        break;
                    }
                }
            }
            ++it;
        }
    }
};

}} // namespace libnest2d::selections

#endif // FIRSTFIT_HPP
