require 'test/unit'
require 'rpm'

class RPM_File_Tests < Test::Unit::TestCase
    def test_link_to_nil
        # FIXME: This currently segfaults
        assert_nothing_raised {
            f = RPM::File.new("path", "md5sum", nil, 42, 1, 
                              "owner", "group", 43, 0777, 44, 45)
        }
    end
end
