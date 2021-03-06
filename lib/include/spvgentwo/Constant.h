#pragma once

#include "Type.h"
#include "Operand.h" // for appendLiteralsToContainer

namespace spvgentwo
{
	class Constant
	{
	public:
		Constant(IAllocator* _pAllocator = nullptr);
		Constant(const Constant& _other);
		Constant(Constant&& _other) noexcept;
		~Constant();

		Constant& operator=(const Constant& _other);
		Constant& operator=(Constant&& _other) noexcept;

		spv::Op getOperation() const { return m_Operation; }
		void setOperation(const spv::Op _op) { m_Operation = _op; }
		const Type& getType() const { return m_Type; }
		Type& getType() { return m_Type; }

		template <class T>
		Type& setType();

		const List<unsigned int>& getData() const { return m_literalData; }
		List<unsigned int>& getData() { return m_literalData; }

		template <class T>
		void addData(const T& _data);

		const List<Constant>& getComponents() const { return m_Components; }
		List<Constant>& getComponents() { return m_Components; }

		template <class T>
		Constant& make(const T& _value, const bool _spec = false);

		// adds a new constituent constant
		Constant& Component();

		void reset();

	private:
		spv::Op m_Operation = spv::Op::OpConstantNull;
		Type m_Type;

		List<Constant> m_Components;
		List<unsigned int> m_literalData;
	};

	template<class T>
	inline Type& Constant::setType()
	{
		return m_Type.make<T>();
	}

	template<class T>
	inline void Constant::addData(const T& _data)
	{
		appendLiteralsToContainer(m_literalData, _data);
	}

	template <class T>
	Constant& Constant::make(const T& _value, const bool _spec)
	{
		if constexpr (stdrep::is_same_v<T, bool>)
		{
			m_Operation = _value ? (_spec ? spv::Op::OpSpecConstantTrue : spv::Op::OpConstantTrue) : (_spec ? spv::Op::OpSpecConstantFalse : spv::Op::OpConstantFalse);
			m_Type.make<bool>();
			appendLiteralsToContainer(m_literalData, _value);
		}
		else if constexpr (traits::is_primitive_type_v<traits::remove_cvref_t<T>>)
		{
			m_Operation = _spec ? spv::Op::OpSpecConstant : spv::Op::OpConstant;
			m_Type.make<T>();
			appendLiteralsToContainer(m_literalData, _value);
		}
		else if constexpr (is_const_null_v<T>)
		{
			m_Type.make<typename T::const_null_type>();
			m_Operation = spv::Op::OpConstantNull; // no literal data
		}
		else if constexpr (is_const_array_v<T>)
		{
			m_Operation = _spec ? spv::Op::OpSpecConstantComposite : spv::Op::OpConstantComposite;
			m_Type.make<typename T::const_array_type>();
			for (unsigned int i = 0u; i < T::Elements; ++i)
			{
				Component().make(_value.data[i]);
			}
		}
		else if constexpr (is_const_vector_v<T>)
		{
			m_Operation = _spec ? spv::Op::OpSpecConstantComposite : spv::Op::OpConstantComposite;
			m_Type.make<typename T::const_vector_type>();
			for (unsigned int i = 0u; i < T::Elements; ++i)
			{
				Component().make(_value.data[i]);
			}
		}
		else if constexpr (is_const_matrix_v<T>)
		{
			m_Operation = _spec ? spv::Op::OpSpecConstantComposite : spv::Op::OpConstantComposite;
			m_Type.make<typename T::const_matrix_type>();
			for (unsigned int i = 0u; i < T::Columns; ++i)
			{
				Component().make(_value.data[i]);
			}
		}
		else if constexpr(is_const_sampler_v<T>)
		{
			m_Operation = spv::Op::OpConstantSampler;
			m_Type.make<typename T::const_sampler_type>();
			appendLiteralsToContainer(m_literalData, _value.addressMode);
			appendLiteralsToContainer(m_literalData, _value.coordMode);
			appendLiteralsToContainer(m_literalData, _value.filterMode);
		}
		else
		{
			// required for clang which can't deduce that this code is unreachable for supported type instantiation
			constexpr bool match =
				stdrep::is_same_v<T, bool> ||
				traits::is_primitive_type_v<traits::remove_cvref_t<T>> ||
				is_const_null_v<T> ||
				is_const_array_v<T> ||
				is_const_vector_v<T> ||
				is_const_matrix_v<T> ||
				is_const_sampler_v<T>;

			static_assert(match, "Could not match type");
		}

		return *this;
	}

	template <>
	struct Hasher<Constant>
	{
		Hash64 operator()(const Constant& _const, FNV1aHasher& _hasher) const
		{
			_hasher << _const.getOperation();
			Hasher<Type>()(_const.getType(), _hasher);
			for (const auto& l : _const.getData())
			{
				_hasher << l;
			}

			for (const Constant& component : _const.getComponents())
			{
				operator()(component, _hasher);
			}

			return _hasher;
		}

		Hash64 operator()(const Constant& _const) const
		{
			FNV1aHasher hasher;
			return operator()(_const, hasher);
		}
	};
} // !spvgentwo