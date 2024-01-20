#ifndef _VARIANTS_H_
#define _VARIANTS_H_
#pragma once

#include <variant>
#include <type_traits>
#include <functional>

namespace custom_view {
  template <class... T> struct always_false : std::false_type {};

  // To have true, but for a type that user code can't reuse as lambda types are unique.
  template <> struct always_false<decltype([]() {}) > : std::true_type {};
   
	template <typename T, typename... Ts>
	struct RemoveType;

	// Base case: Empty type list
	template <typename T>
	struct RemoveType<T> {};

	// Recursive case: Remove T if found, otherwise keep the current type
	template <typename T, typename U, typename... Ts>
	struct RemoveType<T, U, Ts...>
		: std::conditional_t<std::is_same_v<T, U>,
		RemoveType<T, Ts...>,
		std::type_identity<U>> {};

	// Concatenate remaining types after removing T
	template <typename T, typename... Ts>
	using RemoveTypeT = typename RemoveType<T, Ts...>::type;

	// Helper function to create a new variant with the removed type
	template <typename T, typename... Ts>
	auto remove_type_if_helper(const std::variant<Ts...>& v, std::true_type) {
		  // If T is present in the variant, return a variant holding monostate
		  return std::variant<std::monostate>{};
	}

	template <typename T, typename... Ts>
	auto remove_type_if_helper(const std::variant<Ts...>& v, std::false_type) {
		  // Use fold expression to create the new variant with remaining types
		  return std::visit([](auto&&... args) {
			  // Ensure type deduction includes Ts...
			  return std::variant<RemoveTypeT<T, std::decay_t<decltype(args)>, Ts...>...>{std::forward<decltype(args)>(args)...};
			}, v);
	}

	// Function to create a new variant with the removed type
	template <typename T, typename... Ts>
	auto remove_type_if(const std::variant<Ts...>& v) {
		return remove_type_if_helper<T>(v, std::disjunction<std::is_same<T, Ts>...>{});
	}

    // Helper to check if a type exists in a type list
    template <typename T, typename... Ts>
    struct ContainsType;

    template <typename T>
    struct ContainsType<T> : std::false_type {};

    template <typename T, typename U, typename... Ts>
    struct ContainsType<T, U, Ts...> : std::conditional_t<std::is_same_v<T, U>,
        std::true_type,
        ContainsType<T, Ts...>> {};

    template <typename T, typename... Ts>
    constexpr size_t get_index(std::variant<Ts...> const&) {
        size_t r{ 0 };
        auto test = [&](bool b) {
            if (!b) 
                ++r;
            return b;
        }; (test(std::is_same_v<T, Ts>) || ...);
        return r;
    }

  template<class T, class TypeList>
  struct IsContainedIn;
  
	template <typename T, typename TypeList>
	struct IsContainedIn;

	template <typename T, typename... Ts>
	struct IsContainedIn<T, std::variant<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

	template <typename T, typename... Ts>
	constexpr bool is_contained_in_v = std::is_constructible_v<std::variant<Ts...>, T>;

    template <typename T, typename... Ts>
    constexpr bool variant_listed_type( const std::variant<Ts...> &variant_instance ) {
        if constexpr (is_contained_in_v<T, Ts> && IsContainedIn<T, Ts>::value)
            return true;
        return false;
    }

  constexpr std::variant<std::monostate> empty_variant_thunk;

	template <typename... Ts>
    class AfterEmptyHandlerProxy;

	template <typename... Ts>
    class AfterPipeChainHandlerProxy;

  template <typename... Ts>
  struct VariantViewer {
        using variant_t = std::variant<Ts...>;
        VariantViewer(const variant_t& from) : variant_holder(from) {}
        variant_t variant_holder;
        mutable std::unordered_set<std::size_t> processedTypes{};

	template <typename Callable>
        AfterPipeChainHandlerProxy<Ts...> operator|(Callable&& c) {
            	proxy_request_pipe(std::forward<Callable>(c));
		return *this;
	}

        template <typename Callable>
	constexpr auto& proxy_request_pipe(Callable&& c) const {

    		std::visit([&](auto&& val) {
    			using T = decltype(val);
                	if constexpr (std::is_invocable_v<Callable&, T>)
                    		std::invoke(std::forward<Callable>(c), val);
              //  else
                 //   if constexpr (!IsContainedIn<T, std::variant<Ts...>>::value)
    			//		static_assert(always_false<T>::value, "type not listed in variant");
    		}, variant_holder);

	 	return variant_holder;
	}

        // handler for nullopt variant case or regular variant but that monostate contained
         // template <typename Callable>
        //auto operator << (const on_empty& when_empty) const;

        template <typename Callable>
        AfterEmptyHandlerProxy<Ts...> operator << (Callable&& on_empty_callback) const {
            if (empty())
				      on_empty_callback();
            return *this;
        }

        // when no monostate do visit on universal (auto) lambda
        template <typename Callable>
        constexpr bool operator >> (Callable&& callback) const {
            if (!empty()) {
                std::visit(std::forward<Callable>(callback), variant_holder);
                return true;
            } return false;
        }

	template <typename T>
	bool extract_value_to( std::optional<T> &to ) const {
		if (const T* value = std::get_if<T>(&variant_holder)) {
                	to.emplace(*value);
                	return true;
            	} to = std::nullopt;
            return false;
	}

        template <typename T>
        friend bool operator ^= ( std::optional<T> &to, const VariantViewer<Ts...> &this_inst ) {
            return this_inst.extract_value_to(to);
        }

        template <typename T>
        constexpr bool contains() {
            return std::get_if<T>(&variant_holder) != nullptr;
	}

        // nothing to view (nullopt or monostate)
        constexpr bool empty() const {
            return variant_holder.index() == 0;
        }
    };

    template <typename... Ts>
    struct ContainsMonostate : std::disjunction<std::is_same<Ts, std::monostate>...> {};

	template <typename Variant>
	consteval bool mono_state_is_first() {
		if constexpr (std::is_same_v<std::variant_alternative_t<0, Variant>, std::monostate>)
			return true;
		if constexpr (!std::is_same_v<std::variant_alternative_t<0, Variant>, std::monostate>)
			static_assert(always_false<Variant>::value, "variant monostate is not first in type list of passed to variants(viewer)!");
		return false;
	}

	template <typename... Ts>
	consteval bool mono_state_in_list() {
		return is_contained_in_v<std::monostate, Ts...>;
	}

    template <typename... Ts>
    inline VariantViewer<Ts...> CreateViewer(const std::variant<Ts... >& variant) {
        static_assert (ContainsMonostate<Ts...>::value, "should contains monostate");
        if constexpr( mono_state_is_first<std::variant<Ts...>>())
			return { variant };
    }

	template <typename... Ts>
    class AfterPipeChainHandlerProxy {
        VariantViewer<Ts...> variant_viewer_proxy_ref;
    public:
        AfterPipeChainHandlerProxy(const VariantViewer<Ts...>& v) : variant_viewer_proxy_ref(v) {};

		    template <typename Callable>
        AfterPipeChainHandlerProxy<Ts...> operator | (Callable&& callback) const {
            return { variant_viewer_proxy_ref.proxy_request_pipe(std::forward<Callable>(callback)) };
		    }
  };

    
	template <typename... Ts>
	class AfterEmptyHandlerProxy {
		    VariantViewer<Ts...> variant_viewer_proxy_ref;
	  public:
		    AfterEmptyHandlerProxy(const VariantViewer<Ts...>& v) : variant_viewer_proxy_ref(v) {};
       
		    template <typename Callable>
        constexpr bool operator >> (Callable&& callback) const {
            return variant_viewer_proxy_ref.operator>>(std::forward<Callable>(callback));
        }

        template <typename Callable>
        AfterPipeChainHandlerProxy<Ts...> operator | (Callable && callback) const {
            return AfterPipeChainHandlerProxy<Ts...>( variant_viewer_proxy_ref.proxy_request_pipe(std::forward<Callable>(callback)) );
        }
	};

    // TODO: add 'requires' to valid variant types check and static assert for on_type<T> where T is not part of Ts...

    template <typename... Ts>
    constexpr auto variants(const std::variant<Ts... >& variant) {
        if constexpr (mono_state_in_list<Ts...>())
            return CreateViewer(variant);
        else
            return CreateViewer(std::variant<std::monostate, Ts...>{
            std::visit([](auto&& value) { return std::variant<std::monostate, Ts...>{ value };  }, variant) });
    }

    template <typename... Ts>
    constexpr auto variants(const std::optional < std::variant<Ts...>>& variant_opt) {
        if (variant_opt)
            return variants(*variant_opt);
        else
            if constexpr (mono_state_in_list<Ts...>())
                return variants(std::variant<Ts...>{ std::monostate{} });
			else
				return CreateViewer(std::variant<std::monostate, Ts...>{ std::monostate{} });
    }

}

#endif
