#include <gtest/gtest.h>
#include "jsonParser.h"
#include <vector>
#include <string>

class JsonStateTrackerTest : public ::testing::Test {
protected:
    JsonStateTtacker tracker;
};

TEST_F(JsonStateTrackerTest, SimpleJsonObject) {
    std::string json = "{\"name\":\"test\"}";
    bool found = false;
    
    for (size_t i = 0; i < json.size(); ++i) {
        found = tracker.processChar(json[i]);
        if (i < json.size() - 1) {
            EXPECT_FALSE(found);
        }
    }
    
    // Last character should signal completion
    EXPECT_TRUE(found);
    EXPECT_TRUE(tracker.isComplete());
}

TEST_F(JsonStateTrackerTest, NestedJsonObject) {
    std::string json = "{\"data\":{\"name\":\"test\",\"values\":[1,2,3]}}";
    bool found = false;
    
    for (size_t i = 0; i < json.size(); ++i) {
        found = tracker.processChar(json[i]);
        if (i < json.size() - 1) {
            EXPECT_FALSE(found);
        }
    }
    
    EXPECT_TRUE(found);
    EXPECT_TRUE(tracker.isComplete());
}

TEST_F(JsonStateTrackerTest, EscapedQuotes) {
    std::string json = "{\"message\":\"Quote: \\\"Hello\\\"\"}";
    bool found = false;
    
    for (size_t i = 0; i < json.size(); ++i) {
        found = tracker.processChar(json[i]);
        if (i < json.size() - 1) {
            EXPECT_FALSE(found);
        }
    }
    
    EXPECT_TRUE(found);
    EXPECT_TRUE(tracker.isComplete());
}

TEST_F(JsonStateTrackerTest, IncompleteJson) {
    std::string json = "{\"name\":\"test\"";
    
    for (char c : json) {
        tracker.processChar(c);
    }
    
    EXPECT_FALSE(tracker.isComplete());
}

TEST_F(JsonStateTrackerTest, JsonWithArray) {
    std::string json = "[{\"id\":1},{\"id\":2}]";
    bool found = false;
    
    for (size_t i = 0; i < json.size(); ++i) {
        found = tracker.processChar(json[i]);
        if (i < json.size() - 1) {
            EXPECT_FALSE(found);
        }
    }
    
    // Arrays should also be detected as complete JSON
    EXPECT_TRUE(found);
    EXPECT_TRUE(tracker.isComplete());
}

class IncrementalJsonParserTest : public ::testing::Test {
protected:
    std::vector<std::string> received_jsons;
    std::vector<std::string> errors;
    
    std::unique_ptr<JsonParserBase> parser;
    
    void SetUp() override {
        received_jsons.clear();
        errors.clear();
        
        parser = JsonParserFactory::createParser(
            JsonParserFactory::ParserType::INCREMENTAL,
            [this](const std::string& json) { received_jsons.push_back(json); },
            [this](const std::string& error) { errors.push_back(error); }
        );
    }
};

TEST_F(IncrementalJsonParserTest, SingleJson) {
    std::string json = "{\"name\":\"test\"}";
    parser->addData(json);
    
    ASSERT_EQ(1, received_jsons.size());
    EXPECT_EQ(json, received_jsons[0]);
    EXPECT_TRUE(errors.empty());
}

TEST_F(IncrementalJsonParserTest, MultipleJsons) {
    std::string json1 = "{\"id\":1}";
    std::string json2 = "{\"id\":2}";
    
    parser->addData(json1 + json2);
    
    ASSERT_EQ(2, received_jsons.size());
    EXPECT_EQ(json1, received_jsons[0]);
    EXPECT_EQ(json2, received_jsons[1]);
}

TEST_F(IncrementalJsonParserTest, PartialJsons) {
    std::string json = "{\"name\":\"test\"}";
    
    // Send partial data
    parser->addData(json.substr(0, 5));
    EXPECT_TRUE(received_jsons.empty());
    
    // Send the rest
    parser->addData(json.substr(5));
    ASSERT_EQ(1, received_jsons.size());
    EXPECT_EQ(json, received_jsons[0]);
}

TEST_F(IncrementalJsonParserTest, JsonWithWhitespace) {
    
    std::string input = "  {\"id\":1}  \n  {\"id\":2}  ";
    parser->addData(input);
    
    ASSERT_EQ(2, received_jsons.size());
    EXPECT_EQ("{\"id\":1}", received_jsons[0]);
    EXPECT_EQ("{\"id\":2}", received_jsons[1]);
}

class RingBufferJsonParserTest : public ::testing::Test {
protected:
    std::vector<std::string> received_jsons;
    std::vector<std::string> errors;
    
    std::unique_ptr<JsonParserBase> parser;
    
    void SetUp() override {
        received_jsons.clear();
        errors.clear();
        
        parser = JsonParserFactory::createParser(
            JsonParserFactory::ParserType::RING_BUFFER,
            [this](const std::string& json) { received_jsons.push_back(json); },
            [this](const std::string& error) { errors.push_back(error); },
            32  // Small buffer size to test resizing
        );
    }
};

TEST_F(RingBufferJsonParserTest, SingleJson) {
    std::string json = "{\"name\":\"test\"}";
    parser->addData(json);
    
    ASSERT_EQ(1, received_jsons.size());
    EXPECT_EQ(json, received_jsons[0]);
    EXPECT_TRUE(errors.empty());
}

TEST_F(RingBufferJsonParserTest, LargeJson) {
    // Create a large JSON string to test buffer resizing
    std::string large_json = "{\"data\":[";
    for (int i = 0; i < 100000; ++i) {
        if (i > 0) large_json += ",";
        large_json += std::to_string(i);
    }
    large_json += "]}";
    
    // std::cout << "Large JSON size: " << large_json << std::endl;
    parser->addData(large_json);
    
    std::cout << "Received JSON size: " << received_jsons.size() << std::endl;
    ASSERT_EQ(1, received_jsons.size());
    EXPECT_EQ(large_json, received_jsons[0]);
}

TEST_F(RingBufferJsonParserTest, MultipleJsons) {
    std::string json1 = "{\"id\":1}";
    std::string json2 = "{\"id\":2}";
    
    parser->addData(json1);
    parser->addData(json2);
    
    ASSERT_EQ(2, received_jsons.size());
    EXPECT_EQ(json1, received_jsons[0]);
    EXPECT_EQ(json2, received_jsons[1]);
}

TEST_F(RingBufferJsonParserTest, ClearParser) {
    std::string json1 = "{\"id\":1}";
    parser->addData(json1);
    
    ASSERT_EQ(1, received_jsons.size());
    
    parser->clear();
    received_jsons.clear();
    
    std::string json2 = "{\"id\":2}";
    parser->addData(json2);
    
    ASSERT_EQ(1, received_jsons.size());
    EXPECT_EQ(json2, received_jsons[0]);
}

// Test for the parser factory
TEST(JsonParserFactoryTest, CreateIncrementalParser) {
    auto parser = JsonParserFactory::createParser(
        JsonParserFactory::ParserType::INCREMENTAL,
        [](const std::string&) {},
        nullptr
    );
    
    EXPECT_NE(nullptr, parser);
    EXPECT_NO_THROW(parser->addData("{\"test\":true}"));
}

TEST(JsonParserFactoryTest, CreateRingBufferParser) {
    auto parser = JsonParserFactory::createParser(
        JsonParserFactory::ParserType::RING_BUFFER,
        [](const std::string&) {},
        nullptr,
        1024
    );
    
    EXPECT_NE(nullptr, parser);
    EXPECT_NO_THROW(parser->addData("{\"test\":true}"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // ::testing::GTEST_FLAG(filter) = "IncrementalJsonParserTest.JsonWithWhitespace";
    // ::testing::GTEST_FLAG(filter) = "RingBufferJsonParserTest.LargeJson";
    return RUN_ALL_TESTS();
}