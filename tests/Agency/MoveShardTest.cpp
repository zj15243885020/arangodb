////////////////////////////////////////////////////////////////////////////////
/// @brief test case for FailedLeader job
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2017 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Andreas Streichardt
/// @author Copyright 2017, ArangoDB GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "catch.hpp"
#include "fakeit.hpp"

#include "Agency/MoveShard.h"
#include "Agency/AgentInterface.h"
#include "Agency/Node.h"

#include <velocypack/Slice.h>
#include <velocypack/velocypack-aliases.h>

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::consensus;
using namespace fakeit;

const std::string PREFIX = "arango";
const std::string DATABASE = "database";
const std::string COLLECTION = "collection";
const std::string SHARD = "shard";
const std::string SHARD_LEADER = "leader";
const std::string SHARD_FOLLOWER1 = "follower1";
const std::string FREE_SERVER = "free";
const std::string FREE_SERVER2 = "free2";

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::consensus;
using namespace fakeit;

namespace arangodb {
namespace tests {
namespace move_shard_test {

Node createAgencyFromBuilder(VPackBuilder const& builder) {
  Node node("");

  VPackBuilder opBuilder;
  {
    VPackObjectBuilder a(&opBuilder);
    opBuilder.add("new", builder.slice());
  }

  node.handle<SET>(opBuilder.slice());
  return node(PREFIX);
}

#define CHECK_FAILURE(source, query) \
    std::string sourceKey = "/arango/Target/";\
    sourceKey += source;\
    sourceKey += "/1"; \
    REQUIRE(std::string(q->slice().typeName()) == "array"); \
    REQUIRE(q->slice().length() == 1); \
    REQUIRE(std::string(q->slice()[0].typeName()) == "array"); \
    REQUIRE(q->slice()[0].length() == 1); \
    REQUIRE(std::string(q->slice()[0][0].typeName()) == "object"); \
    auto writes = q->slice()[0][0]; \
    REQUIRE(std::string(writes.get(sourceKey).typeName()) == "object"); \
    REQUIRE(std::string(writes.get(sourceKey).get("op").typeName()) == "string"); \
    CHECK(writes.get(sourceKey).get("op").copyString() == "delete"); \
    CHECK(std::string(writes.get("/arango/Target/Failed/1").typeName()) == "object");

Node createRootNode() {
  Node root("ROOT");

  VPackBuilder builder;
  {
    VPackObjectBuilder a(&builder);
    builder.add(VPackValue("new"));
    {
      VPackObjectBuilder a(&builder);
      builder.add(VPackValue(PREFIX));
      {
        VPackObjectBuilder b(&builder);
        builder.add(VPackValue("Target"));
        {
          VPackObjectBuilder c(&builder);
          builder.add(VPackValue("ToDo"));
          {
            VPackObjectBuilder d(&builder);
          }
          builder.add(VPackValue("Finished"));
          {
            VPackObjectBuilder d(&builder);
          }
          builder.add(VPackValue("Failed"));
          {
            VPackObjectBuilder d(&builder);
          }
          builder.add(VPackValue("FailedServers"));
          {
            VPackObjectBuilder d(&builder);
          }
          builder.add(VPackValue("CleanedServers"));
          {
            VPackArrayBuilder d(&builder);
          }
        }
        builder.add(VPackValue("Current"));
        {
          VPackObjectBuilder c(&builder);
          builder.add(VPackValue("Collections"));
          {
            VPackObjectBuilder d(&builder);
            builder.add(VPackValue(DATABASE));
            {
              VPackObjectBuilder e(&builder);
              builder.add(VPackValue(COLLECTION));
              {
                VPackObjectBuilder f(&builder);
                builder.add(VPackValue(SHARD));
                {
                  VPackObjectBuilder f(&builder);
                  builder.add(VPackValue("servers"));
                  {
                    VPackArrayBuilder g(&builder);
                    builder.add(VPackValue(SHARD_LEADER));
                    builder.add(VPackValue(SHARD_FOLLOWER1));
                  }
                }
              }
            }
          }
        }
        builder.add(VPackValue("Plan"));
        {
          VPackObjectBuilder c(&builder);
          builder.add(VPackValue("Collections"));
          {
            VPackObjectBuilder d(&builder);
            builder.add(VPackValue(DATABASE));
            {
              VPackObjectBuilder e(&builder);
              builder.add(VPackValue(COLLECTION));
              {
                VPackObjectBuilder f(&builder);
                builder.add(VPackValue("shards"));
                {
                  VPackObjectBuilder f(&builder);
                  builder.add(VPackValue(SHARD));
                  {
                    VPackArrayBuilder g(&builder);
                    builder.add(VPackValue(SHARD_LEADER));
                    builder.add(VPackValue(SHARD_FOLLOWER1));
                  }
                }
              }
            }
          }
          builder.add(VPackValue("DBServers"));
          {
            VPackObjectBuilder d(&builder);
            builder.add(SHARD_LEADER, VPackValue("none"));
            builder.add(SHARD_FOLLOWER1, VPackValue("none"));
            builder.add(FREE_SERVER, VPackValue("none"));
            builder.add(FREE_SERVER2, VPackValue("none"));
          }
        }
        builder.add(VPackValue("Supervision"));
        {
          VPackObjectBuilder c(&builder);
          builder.add(VPackValue("Health"));
          {
            VPackObjectBuilder c(&builder);
            builder.add(VPackValue(SHARD_LEADER));
            {
              VPackObjectBuilder e(&builder);
              builder.add("Status", VPackValue("GOOD"));
            }
            builder.add(VPackValue(SHARD_FOLLOWER1));
            {
              VPackObjectBuilder e(&builder);
              builder.add("Status", VPackValue("GOOD"));
            }
            builder.add(VPackValue(FREE_SERVER));
            {
              VPackObjectBuilder e(&builder);
              builder.add("Status", VPackValue("GOOD"));
            }
            builder.add(VPackValue(FREE_SERVER2));
            {
              VPackObjectBuilder e(&builder);
              builder.add("Status", VPackValue("GOOD"));
            }
          }
          builder.add(VPackValue("DBServers"));
          {
            VPackObjectBuilder c(&builder);
          }
          builder.add(VPackValue("Shards"));
          {
            VPackObjectBuilder c(&builder);
          }
        }
      }
    }
  }
  root.handle<SET>(builder.slice());
  return root;
}

VPackBuilder createJob(std::string const& shard, std::string const& from, std::string const& to) {
  VPackBuilder builder;
  {
    VPackObjectBuilder b(&builder);
    builder.add("jobId", VPackValue("1"));
    builder.add("creator", VPackValue("unittest"));
    builder.add("type", VPackValue("moveShard"));
    builder.add("database", VPackValue(DATABASE));
    builder.add("collection", VPackValue(COLLECTION));
    builder.add("shard", VPackValue(SHARD));
    builder.add("fromServer", VPackValue(from));
    builder.add("toServer", VPackValue(to));
    builder.add("isLeader", VPackValue(from == SHARD_LEADER));
  }
  return builder;
}

TEST_CASE("MoveShard", "[agency][supervision]") {
auto baseStructure = createRootNode();
write_ret_t fakeWriteResult {true, "", std::vector<bool> {true}, std::vector<index_t> {1}};
std::string const jobId = "1";

SECTION("the job should fail if toServer does not exist") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, SHARD_LEADER, "unfug").slice());
      }
      builder->close();
    } else {
      builder->add(s);
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  AgentInterface& agent = mockAgent.get();
  When(Method(mockAgent, write)).AlwaysDo([&](query_t const& q) -> write_ret_t {
    INFO("WriteTransaction: " << q->slice().toJson());
    CHECK_FAILURE("ToDo", q);
    return fakeWriteResult;
  });
  When(Method(mockAgent, waitFor)).AlwaysReturn();
  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should fail if fromServer does not exist") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, "unfug", FREE_SERVER).slice());
      }
      builder->close();
    } else {
      builder->add(s);
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  AgentInterface& agent = mockAgent.get();
  When(Method(mockAgent, write)).AlwaysDo([&](query_t const& q) -> write_ret_t {
    INFO("WriteTransaction: " << q->slice().toJson());
    CHECK_FAILURE("ToDo", q);
    return fakeWriteResult;
  });
  When(Method(mockAgent, waitFor)).AlwaysReturn();
  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should fail if fromServer is not in plan of the shard") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, FREE_SERVER, FREE_SERVER2).slice());
      }
      builder->close();
    } else {
      builder->add(s);
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  AgentInterface& agent = mockAgent.get();
  When(Method(mockAgent, write)).AlwaysDo([&](query_t const& q) -> write_ret_t {
    INFO("WriteTransaction: " << q->slice().toJson());
    CHECK_FAILURE("ToDo", q);
    return fakeWriteResult;
  });
  When(Method(mockAgent, waitFor)).AlwaysReturn();
  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should fail if fromServer does not exist") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, "unfug", FREE_SERVER).slice());
      }
      builder->close();
    } else {
      builder->add(s);
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  AgentInterface& agent = mockAgent.get();
  When(Method(mockAgent, write)).AlwaysDo([&](query_t const& q) -> write_ret_t {
    INFO("WriteTransaction: " << q->slice().toJson());
    REQUIRE(std::string(q->slice().typeName()) == "array" );
    REQUIRE(q->slice().length() == 1);
    REQUIRE(std::string(q->slice()[0].typeName()) == "array");
    REQUIRE(q->slice()[0].length() == 1); // we always simply override! no preconditions...
    REQUIRE(std::string(q->slice()[0][0].typeName()) == "object");

    auto writes = q->slice()[0][0];
    REQUIRE(std::string(writes.get("/arango/Target/ToDo/1").typeName()) == "object");
    REQUIRE(std::string(writes.get("/arango/Target/ToDo/1").get("op").typeName()) == "string");
    CHECK(writes.get("/arango/Target/ToDo/1").get("op").copyString() == "delete");
    CHECK(std::string(writes.get("/arango/Target/Failed/1").typeName()) == "object");
    return fakeWriteResult;
  });
  When(Method(mockAgent, waitFor)).AlwaysReturn();
  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should remain in todo if the shard is currently locked") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, SHARD_LEADER, FREE_SERVER).slice());
      } else if (path == "/arango/Supervision/Shards") {
        builder->add(SHARD, VPackValue("2"));
      }
      builder->close();
    } else {
      builder->add(s);
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  // nothing should be called (job remains in ToDo)
  AgentInterface& agent = mockAgent.get();

  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should remain in todo if the target server is currently locked") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, SHARD_LEADER, FREE_SERVER).slice());
      } else if (path == "/arango/Supervision/DBServers") {
        builder->add(FREE_SERVER, VPackValue("2"));
      }
      builder->close();
    } else {
      builder->add(s);
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  // nothing should be called (job remains in ToDo)
  AgentInterface& agent = mockAgent.get();

  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should fail if the target server was cleaned out") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, SHARD_LEADER, FREE_SERVER).slice());
      }
      builder->close();
    } else {
      if (path == "/arango/Target/CleanedServers") {
        builder->add(VPackValue(VPackValueType::Array));
        builder->add(VPackValue(FREE_SERVER));
        builder->close();
      } else {
        builder->add(s);
      }
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  When(Method(mockAgent, write)).AlwaysDo([&](query_t const& q) -> write_ret_t {
    INFO("WriteTransaction: " << q->slice().toJson());
    CHECK_FAILURE("ToDo", q);
    return fakeWriteResult;
  });
  When(Method(mockAgent, waitFor)).AlwaysReturn();
  AgentInterface& agent = mockAgent.get();

  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should fail if the target server was cleaned out") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, SHARD_LEADER, FREE_SERVER).slice());
      }
      builder->close();
    } else {
      if (path == "/arango/Target/CleanedServers") {
        builder->add(VPackValue(VPackValueType::Array));
        builder->add(VPackValue(FREE_SERVER));
        builder->close();
      } else {
        builder->add(s);
      }
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  When(Method(mockAgent, write)).AlwaysDo([&](query_t const& q) -> write_ret_t {
    INFO("WriteTransaction: " << q->slice().toJson());
    CHECK_FAILURE("ToDo", q);
    return fakeWriteResult;
  });
  When(Method(mockAgent, waitFor)).AlwaysReturn();

  AgentInterface& agent = mockAgent.get();

  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

SECTION("the job should be moved to pending when everything is ok") {
  std::function<std::unique_ptr<VPackBuilder>(VPackSlice const&, std::string const&)> createTestStructure = [&](VPackSlice const& s, std::string const& path) {
    std::unique_ptr<VPackBuilder> builder;
    builder.reset(new VPackBuilder());
    if (s.isObject()) {
      builder->add(VPackValue(VPackValueType::Object));
      for (auto const& it: VPackObjectIterator(s)) {
        auto childBuilder = createTestStructure(it.value, path + "/" + it.key.copyString());
        if (childBuilder) {
          builder->add(it.key.copyString(), childBuilder->slice());
        }
      }

      if (path == "/arango/Target/ToDo") {
        builder->add(jobId, createJob(SHARD, SHARD_LEADER, FREE_SERVER).slice());
      }
      builder->close();
    } else {
      builder->add(s);
    }
    return builder;
  };

  Mock<AgentInterface> mockAgent;
  When(Method(mockAgent, write)).AlwaysDo([&](query_t const& q) -> write_ret_t {
    INFO("WriteTransaction: " << q->slice().toJson());
    std::string sourceKey = "/arango/Target/ToDo/1";
    REQUIRE(std::string(q->slice().typeName()) == "array");
    REQUIRE(q->slice().length() == 1);
    REQUIRE(std::string(q->slice()[0].typeName()) == "array");
    REQUIRE(q->slice()[0].length() == 2);
    REQUIRE(std::string(q->slice()[0][0].typeName()) == "object");
    REQUIRE(std::string(q->slice()[0][1].typeName()) == "object");

    auto writes = q->slice()[0][0];
    REQUIRE(std::string(writes.get(sourceKey).typeName()) == "object");
    REQUIRE(std::string(writes.get(sourceKey).get("op").typeName()) == "string");
    CHECK(writes.get(sourceKey).get("op").copyString() == "delete");
    CHECK(std::string(writes.get("/arango/Target/Pending/1").typeName()) == "object");
    CHECK(std::string(writes.get("/arango/Target/Pending/1/timeStarted").typeName()) == "object");
    CHECK(writes.get("/arango/Supervision/Shards/" + SHARD).copyString() == "1");
    CHECK(writes.get("/arango/Supervision/DBServer/" + FREE_SERVER).copyString() == "1");
    CHECK(writes.get("/arango/Plan/Version").get("op").copyString() == "increment");

    auto preconditions = q->slice()[0][1];
    CHECK(preconditions.get("/arango/Target/CleanedServers").get("old").toJson() == "[]");
    CHECK(preconditions.get("/arango/Target/FailedServers").get("old").toJson() == "{}");
    CHECK(preconditions.get("/arango/Supervision/DBServers/" + FREE_SERVER).get("oldEmpty").getBool() == true);
    CHECK(preconditions.get("/arango/Supervision/Shards/" + SHARD).get("oldEmpty").getBool() == true);
    CHECK(preconditions.get("/arango/Plan/Collections/" + DATABASE + "/" + COLLECTION + "/shards/" + SHARD).get("old").toJson() == "[\"" + SHARD_LEADER + "\",\"" + SHARD_FOLLOWER1 + "\"]");

    return fakeWriteResult;
  });
  When(Method(mockAgent, waitFor)).AlwaysReturn();

  AgentInterface& agent = mockAgent.get();

  auto builder = createTestStructure(baseStructure.toBuilder().slice(), "");
  REQUIRE(builder);
  Node agency = createAgencyFromBuilder(*builder);

  INFO("Agency: " << agency);
  auto moveShard = MoveShard(agency, &agent, TODO, jobId);
  moveShard.start();
}

}

}
}
}
