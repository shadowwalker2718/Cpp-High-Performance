/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_GROUP_BY_HPP
#define RANGES_V3_VIEW_GROUP_BY_HPP

#include <utility>
#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/range.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/view_facade.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/semiregular.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/algorithm/adjacent_find.hpp>
#include <range/v3/view/view.hpp>
#include <range/v3/view/take_while.hpp>

namespace ranges
{
    inline namespace v3
    {
        // TODO group_by could support Input ranges by keeping mutable state in
        // the range itself. The group_by view would then be mutable-only and
        // Input.

        /// \addtogroup group-views
        /// @{
        template<typename Rng, typename Fun>
        struct group_by_view
          : view_facade<
                group_by_view<Rng, Fun>,
                is_finite<Rng>::value ? finite : range_cardinality<Rng>::value>
        {
        private:
            friend range_access;
            Rng rng_;
            semiregular_t<function_type<Fun>> fun_;

            template<bool IsConst>
            struct cursor
            {
            private:
                friend range_access; friend group_by_view;
                range_iterator_t<Rng> cur_;
                range_sentinel_t<Rng> last_;
                semiregular_ref_or_val_t<function_type<Fun>, IsConst> fun_;

                struct take_while_pred
                {
                    range_iterator_t<Rng> first_;
                    semiregular_ref_or_val_t<function_type<Fun>, IsConst> fun_;
                    bool operator()(range_reference_t<Rng> ref) const
                    {
                        return fun_(*first_, ref);
                    }
                };
                take_while_view<range<range_iterator_t<Rng>, range_sentinel_t<Rng>>, take_while_pred>
                get() const
                {
                    return {{cur_, last_}, {cur_, fun_}};
                }
                void next()
                {
                    cur_ = ranges::next(adjacent_find(cur_, last_, not_(std::ref(fun_))), 1, last_);
                }
                bool done() const
                {
                    return cur_ == last_;
                }
                bool equal(cursor const &that) const
                {
                    return cur_ == that.cur_;
                }
                cursor(semiregular_ref_or_val_t<function_type<Fun>, IsConst> fun, range_iterator_t<Rng> first,
                    range_sentinel_t<Rng> last)
                  : cur_(first), last_(last), fun_(fun)
                {}
            public:
                cursor() = default;
            };
            cursor<false> begin_cursor()
            {
                return {fun_, ranges::begin(rng_), ranges::end(rng_)};
            }
#ifdef RANGES_WORKAROUND_MSVC_SFINAE_CONSTEXPR
            CONCEPT_REQUIRES(Callable<Fun const, range_common_reference_t<Rng>,
                range_common_reference_t<Rng>>::value && Range<Rng const>::value)
#else
            CONCEPT_REQUIRES(Callable<Fun const, range_common_reference_t<Rng>,
                range_common_reference_t<Rng>>() && Range<Rng const>())
#endif
            cursor<true> begin_cursor() const
            {
                return {fun_, ranges::begin(rng_), ranges::end(rng_)};
            }
        public:
            group_by_view() = default;
            group_by_view(Rng rng, Fun fun)
              : rng_(std::move(rng))
              , fun_(std::move(fun))
            {}
        };

        namespace view
        {
            struct group_by_fn
            {
            private:
                friend view_access;
                template<typename Fun>
                static auto bind(group_by_fn group_by, Fun fun)
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    make_pipeable(std::bind(group_by, std::placeholders::_1, std::move(fun)))
                )
            public:
                template<typename Rng, typename Fun>
                using Concept = meta::and_<
                    ForwardRange<Rng>,
                    IndirectCallablePredicate<Fun, range_iterator_t<Rng>,
                        range_iterator_t<Rng>>>;

                template<typename Rng, typename Fun,
#ifdef RANGES_WORKAROUND_MSVC_SFINAE_CONSTEXPR
                    CONCEPT_REQUIRES_(Concept<Rng, Fun>::value)>
#else
                    CONCEPT_REQUIRES_(Concept<Rng, Fun>())>
#endif
                group_by_view<all_t<Rng>, Fun> operator()(Rng && rng, Fun fun) const
                {
                    return {all(std::forward<Rng>(rng)), std::move(fun)};
                }

            #ifndef RANGES_DOXYGEN_INVOKED
                template<typename Rng, typename Fun,
#ifdef RANGES_WORKAROUND_MSVC_SFINAE_CONSTEXPR
                    CONCEPT_REQUIRES_(!Concept<Rng, Fun>::value)>
#else
                    CONCEPT_REQUIRES_(!Concept<Rng, Fun>())>
#endif
                void operator()(Rng &&, Fun) const
                {
                    CONCEPT_ASSERT_MSG(ForwardRange<Rng>(),
                        "The object on which view::group_by operates must be a model of the "
                        "ForwardRange concept.");
                    CONCEPT_ASSERT_MSG(IndirectCallablePredicate<Fun, range_iterator_t<Rng>,
                        range_iterator_t<Rng>>(),
                        "The function passed to view::group_by must be callable with two arguments "
                        "of the range's common reference type, and its return type must be "
                        "convertible to bool.");
                }
            #endif
            };

            /// \relates group_by_fn
            /// \ingroup group-views
            namespace
            {
                constexpr auto&& group_by = static_const<view<group_by_fn>>::value;
            }
        }
        /// @}
    }
}

#endif
