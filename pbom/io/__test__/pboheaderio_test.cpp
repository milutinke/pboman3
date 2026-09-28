#include "io/pboheaderio.h"
#include <QByteArray>
#include <QSharedPointer>
#include <QTemporaryFile>
#include <QtEndian>
#include <gtest/gtest.h>

namespace pboman3::io::test {
    // ReSharper disable once CppInconsistentNaming
    class PboHeaderIOTest_ReadNextEntry : public testing::TestWithParam<int> {
    };

    TEST_P(PboHeaderIOTest_ReadNextEntry, Returns_Null) {
        const int bytesCount = GetParam();

        QByteArray arr;
        arr.append(bytesCount, 0);

        QTemporaryFile t;
        ASSERT_TRUE(t.open());
        t.write(arr);
        t.close();

        PboFile f(t.fileName());
        f.open(QIODeviceBase::ReadOnly);

        const PboHeaderIO io(&f);

        const QSharedPointer<PboNodeEntity> e = io.readNextEntry();
        ASSERT_FALSE(e);
    }

    INSTANTIATE_TEST_SUITE_P(ReadNextEntry, PboHeaderIOTest_ReadNextEntry, testing::Values(0, 10, 20));

    TEST(PboHeaderIOTest, ReadNextEntry_Returns_Boundary) {
        QByteArray arr;
        arr.append(21, 0);

        QTemporaryFile t;
        ASSERT_TRUE(t.open());
        t.write(arr);
        t.close();

        PboFile f(t.fileName());
        f.open(QIODeviceBase::ReadOnly);

        const PboHeaderIO io(&f);

        const QSharedPointer<PboNodeEntity> e = io.readNextEntry();
        ASSERT_TRUE(e);
        ASSERT_TRUE(e->isBoundary());
    }

    // ReSharper disable once CppInconsistentNaming
    class PboHeaderIOTest_ReadNextHeader : public testing::TestWithParam<int> {
    };

    TEST_P(PboHeaderIOTest_ReadNextHeader, Returns_Null) {
        const int bytesCount = GetParam();

        QByteArray arr;
        arr.append(bytesCount, 0);

        QTemporaryFile t;
        ASSERT_TRUE(t.open());
        t.write(arr);
        t.close();

        PboFile f(t.fileName());
        f.open(QIODeviceBase::ReadOnly);

        const PboHeaderIO io(&f);

        const QSharedPointer<PboHeaderEntity> e = io.readNextHeader();
        ASSERT_FALSE(e);
    }

    INSTANTIATE_TEST_SUITE_P(ReadNextHeader, PboHeaderIOTest_ReadNextHeader, testing::Values(0));

    TEST(PboHeaderIOTest, WriteEntry_Writes) {
        const PboNodeEntity e1("some-file1", PboPackingMethod::Packed, 0x01010101, 0x02020202, 0x03030303, 0x04040404);
        const PboNodeEntity e2("some-file2", PboPackingMethod::Uncompressed, 0x05050505, 0x06060606, 0x07070707, 0x08080808);
        QTemporaryFile t;
        ASSERT_TRUE(t.open());

        PboFile f{t.fileName()};
        f.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&f);
        io.writeEntry(e1);
        io.writeEntry(e2);
        f.close();
        
        const QByteArray all = t.readAll();

        QByteArray expected;
        const auto appendLe32 = [&expected](const quint32 value) {
            const quint32 littleEndian = qToLittleEndian(value);
            expected.append(reinterpret_cast<const char*>(&littleEndian), sizeof littleEndian);
        };
        expected.append(e1.fileName().toUtf8()).append(1, 0);
        appendLe32(static_cast<quint32>(e1.packingMethod()));
        appendLe32(e1.originalSize());
        appendLe32(e1.reserved());
        appendLe32(e1.timestamp());
        appendLe32(e1.dataSize());

        expected.append(e2.fileName().toUtf8()).append(1, 0);
        appendLe32(static_cast<quint32>(e2.packingMethod()));
        appendLe32(e2.originalSize());
        appendLe32(e2.reserved());
        appendLe32(e2.timestamp());
        appendLe32(e2.dataSize());

        ASSERT_EQ(expected.compare(all), 0);
    }

    TEST(PboHeaderIOTest, WriteHeader_Writes) {
        const PboHeaderEntity h1("h1", "v1");
        const PboHeaderEntity h2("h2", "v2");
        const PboHeaderEntity h3 = PboHeaderEntity::makeBoundary();
        QTemporaryFile t;
        ASSERT_TRUE(t.open());

        PboFile f{t.fileName()};
        f.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&f);
        io.writeHeader(h1);
        io.writeHeader(h2);
        io.writeHeader(h3);
        f.close();
        
        const QByteArray all = t.readAll();

        QByteArray expected;
        expected.append(h1.name.toUtf8()).append(1, 0);
        expected.append(h1.value.toUtf8()).append(1, 0);
        expected.append(h2.name.toUtf8()).append(1, 0);
        expected.append(h2.value.toUtf8()).append(1, 0);
        expected.append(1, 0);

        ASSERT_EQ(expected.compare(all), 0);
    }
}
