#pragma once

#include "spvgentwo/Writer.h"

namespace spvgentwo
{
	template <typename U32Vector>
	class BinaryVectorWriter : public IWriter
	{
	public:
		BinaryVectorWriter(U32Vector& _vector) : m_vector(_vector) {};
		virtual ~BinaryVectorWriter() {};

		void put(unsigned int _word) final;

	private:
		U32Vector& m_vector;
	};

	template<typename U32Vector>
	inline void BinaryVectorWriter<U32Vector>::put(unsigned int _word)
	{
		m_vector.emplace_back(_word);
	}
} //!spvgentwo
